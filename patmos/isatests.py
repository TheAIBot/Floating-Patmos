import subprocess
import os 
import filecmp
from shutil import copyfile
import asyncio
from concurrent.futures import ProcessPoolExecutor
import multiprocessing
import struct

curr_dir = os.getcwd()

tests_dir = os.path.join(curr_dir, "asm-tests")
isa_gen_dir = os.path.join(tests_dir, "ISATestsGen")
isa_sim_dir = os.path.join(tests_dir, "ISASim")
paasm_build_dir = os.path.join(curr_dir, "simulator", "build")
paasm_exe_dir = os.path.join(paasm_build_dir, "src")
emu_exe_dir = os.path.join(curr_dir, "hardware", "build")

tests_asm_dir = os.path.join(tests_dir, "tests", "asm")
tests_bin_dir = os.path.join(tests_dir, "tests", "bin")
tests_expected_dir = os.path.join(tests_dir, "tests", "expected")
tests_emu_actual_dir = os.path.join(tests_dir, "tests", "emu-actual")
tests_sim_actual_dir = os.path.join(tests_dir, "tests", "isa-actual")
tests_hw_actual_dir = os.path.join(tests_dir, "tests", "hw-actual")
tests_logs_dir = os.path.join(tests_dir, "tests", "logs")

paasm_exe = os.path.join(paasm_exe_dir, "paasm")
isa_sim_exe = os.path.join(isa_sim_dir, "ISASim", "ISASim")
emu_exe = os.path.join(emu_exe_dir, "emulator")

#compile isa gen
subprocess.check_call(["cmake", "CMakeLists.txt"], cwd=isa_gen_dir)
subprocess.check_call(["make"], cwd=isa_gen_dir)

#compile isa sim
subprocess.check_call(["cmake", "CMakeLists.txt"], cwd=isa_sim_dir)
subprocess.check_call(["make"], cwd=isa_sim_dir)

#compile paasm
subprocess.check_call(["make", "paasm"], cwd=paasm_build_dir)

#make test directories
os.makedirs(tests_asm_dir, exist_ok=True)
os.makedirs(tests_bin_dir, exist_ok=True)
os.makedirs(tests_expected_dir, exist_ok=True)
os.makedirs(tests_emu_actual_dir, exist_ok=True)
os.makedirs(tests_sim_actual_dir, exist_ok=True)
os.makedirs(tests_hw_actual_dir, exist_ok=True)
os.makedirs(tests_logs_dir, exist_ok=True)

#generate tests
subprocess.check_call([os.path.join(isa_gen_dir, "ISATestsGen", "ISATestsGen"), tests_asm_dir, tests_expected_dir])

#compile emulator
copyfile(os.path.join(tests_dir, "uartForTests.s"), os.path.join(curr_dir, "asm", "uartForTests.s"))
subprocess.check_call(["make", "emulator", "BOOTAPP=uartForTests"])

class debug_symbols:
    def __init__(self, r, t):
        self.reg = r
        self.type = t



def prety_write_uart(log_handle, uart_file, debug_symbols):
    with open(uart_file, "rb") as uart_handle:
        for symbol in debug_symbols:
            bytes4 = uart_handle.read(4)
            for b in bytes4:
                log_handle.write("{0:08b} ".format(b))
            log_handle.write(": ")

            bytes4 = bytes4[::-1]
            if symbol.type == "int":
                log_handle.write(str(struct.unpack("i", bytes4)[0]) + "\n")
            elif symbol.type == "bool":
                log_handle.write(str(struct.unpack("i", bytes4)[0]) + "\n")
            elif symbol.type == "float":
                num = log_handle.write(str(struct.unpack("f", bytes4)[0]) + "\n")
                if num == 1:
                    log_handle.write("True\n")
                else:
                    log_handle.write("False\n")
            else:
                raise Exception("unexpected debug symbol type. Type: " + symbol.type)

def load_debug_symbols(debug_file):
    symbols = []
    with open(debug_file, "r") as debug_handle:
        line = debug_handle.readline()
        split_line = line.split(":")
        symbols.append(debug_symbols(split_line[0].strip(), split_line[1].strip()))
    return symbols


def write_uart_diff(log_handle, expected_uart_file, actual_uart_file, debug_file):
    symbols = load_debug_symbols(debug_file)

    log_handle.write("Expected:\n")
    prety_write_uart(log_handle, expected_uart_file, symbols)
    log_handle.write("Actual:\n")
    prety_write_uart(log_handle, actual_uart_file, symbols)

def run_test(dirpath, filename, test_name):
    filepath = os.path.join(dirpath, filename)
    basename = os.path.splitext(filename)[0]
    log_file = os.path.join(tests_logs_dir, test_name, basename + ".s")
    debug_file = os.path.join(tests_expected_dir, test_name, basename + ".debug")
    file_bin = os.path.join(tests_bin_dir, test_name, basename + ".bin")
    sim_uart = os.path.join(tests_sim_actual_dir, test_name, basename + ".uart")
    exp_uart = os.path.join(tests_expected_dir, test_name, basename + ".uart")
    emu_uart = os.path.join(tests_emu_actual_dir, test_name, basename + ".uart")

    os.makedirs(os.path.join(tests_bin_dir, test_name), exist_ok=True)
    os.makedirs(os.path.join(tests_emu_actual_dir, test_name), exist_ok=True)
    os.makedirs(os.path.join(tests_sim_actual_dir, test_name), exist_ok=True)
    os.makedirs(os.path.join(tests_hw_actual_dir, test_name), exist_ok=True)
    os.makedirs(os.path.join(tests_logs_dir, test_name), exist_ok=True)

    #write everything to log file
    with open(log_file, "w") as log_handle:
        #compile test
        if subprocess.call([paasm_exe, filepath, file_bin], stdin=subprocess.DEVNULL, stdout=log_handle, stderr=log_handle) != 0:
            return 'C'

        #simulate test
        if subprocess.call([isa_sim_exe, file_bin, sim_uart], stdin=subprocess.DEVNULL, stdout=log_handle, stderr=log_handle) != 0:
            return 'S'
        #compare simulation result
        if not filecmp.cmp(exp_uart, sim_uart):
            write_uart_diff(log_handle, exp_uart, sim_uart, debug_file)
            return 'S'

        #emulate test
        subprocess.call(["-i", "-l", "2000000", "-O", emu_uart, "-b", file_bin], stdin=subprocess.DEVNULL, executable=emu_exe, stdout=log_handle, stderr=log_handle)
        #compare emulation result
        if not filecmp.cmp(exp_uart, emu_uart):
            write_uart_diff(log_handle, exp_uart, emu_uart, debug_file)
            return 'E'

    os.remove(log_file)
    return '-'

async def run_tests_in_parallel(loop):
    executor = ProcessPoolExecutor(max_workers=multiprocessing.cpu_count())
    
    tests_failed = 0
    tests_succeeded = 0

    for dirpath, dirnames, filenames in os.walk(tests_asm_dir):
        #only interested in leaf directories
        if dirnames:
            continue

        test_name = os.path.relpath(dirpath, tests_asm_dir)
        print(format("Testing {} [".format(test_name.ljust(40, ' '))), end='', flush=True)

        test_results = await asyncio.gather(*(loop.run_in_executor(executor, run_test, dirpath, filename, test_name)
                                                for filename in filenames))
        for test_result in test_results:
            if test_result == '-':
                tests_succeeded = tests_succeeded + 1
            else:
                tests_failed = tests_failed + 1
            print(test_result, end='')
        print("]")

    print("Tests completed: {}".format(tests_failed + tests_succeeded))
    print("Tests failed:    {}".format(tests_failed))
    print("Tests succeeded: {}".format(tests_succeeded))

loop = asyncio.get_event_loop()
loop.run_until_complete(run_tests_in_parallel(loop))




