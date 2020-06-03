import subprocess
import filecmp
import os 
from shutil import copyfile

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

tests_failed = 0
tests_succeeded = 0

for dirpath, dirnames, filenames in os.walk(tests_asm_dir):
    #only interested in leaf directories
    if dirnames:
        continue

    test_name = os.path.relpath(dirpath, tests_asm_dir)
    print(format("Testing {} [".format(test_name.ljust(40, ' '))), end='', flush=True)
    for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        basename = os.path.splitext(filename)[0]
        log_file = os.path.join(tests_logs_dir, test_name, basename + ".s")
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
        with open(log_file, "x") as log_handle:
            #compile test
            if subprocess.call([paasm_exe, filepath, file_bin], stdout=log_handle, stderr=log_handle) == 0:
                subprocess.check_call(["touch", os.path.join(tests_bin_dir, test_name, basename + ".dat")])
            else:
                tests_failed = tests_failed + 1
                print('C', end='', flush=True)
                continue

            #simulate test
            if subprocess.call([isa_sim_exe, file_bin, sim_uart], stdout=log_handle, stderr=log_handle) != 0:
                tests_failed = tests_failed + 1
                print('S', end='', flush=True)
                print("Failed to simulator binary", file=log_handle)
                continue
            
            #compare simulation result
            if not filecmp.cmp(exp_uart, sim_uart):
                tests_failed = tests_failed + 1
                print('S', end='', flush=True)

                #with open(log_file, "a") as logf:
                #    logf.write("Simulator output is incorrect")
                #    logf.write("expected")
                #    #print expected here
                #    logf.write("actual")
                #    #print actual here
                continue

            #emulate test
            subprocess.call(["-i", "-l", "2000000", "-O", emu_uart, "-b", file_bin], executable=emu_exe, stdout=log_handle, stderr=log_handle)

            #compare emulation result
            if not filecmp.cmp(exp_uart, emu_uart):
                tests_failed = tests_failed + 1
                print('E', end='', flush=True)

                #with open(log_file, "a") as logf:
                #    logf.write("Emulator output is incorrect")
                #    logf.write("expected")
                #    #print expected here
                #    logf.write("actual")
                #    #print actual here
                continue

            tests_succeeded = tests_succeeded + 1
            print("-", end='', flush=True)
    print("]")

print("Tests completed: {}".format(tests_failed + tests_succeeded))
print("Tests failed:    {}".format(tests_failed))
print("Tests succeeded: {}".format(tests_succeeded))




