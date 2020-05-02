#!/bin/bash

ISA_GEN_DIR=$(pwd)/asm-tests/ISATestsGen/ISATestsGen
ISA_SIM_DIR=$(pwd)/asm-tests/ISASim/ISASim
PAASM_BUILD_DIR=$(pwd)/simulator/build
PAASM_EXE_DIR=$PAASM_BUILD_DIR/src

TESTS_ASM_DIR=$(pwd)/asm-tests/tests/asm
TESTS_BIN_DIR=$(pwd)/asm-tests/tests/bin
TESTS_EXPECTED_DIR=$(pwd)/asm-tests/tests/expected
TESTS_EMU_ACTUAL_DIR=$(pwd)/asm-tests/tests/emu-actual
TESTS_SIM_ACTUAL_DIR=$(pwd)/asm-tests/tests/isa-actual
TESTS_HW_ACTUAL_DIR=$(pwd)/asm-tests/tests/hw-actual

LOG_FILE=$(pwd)/testlog.log


#compile programs
pushd $ISA_GEN_DIR && cmake CMakeLists.txt && make && popd || exit 1
pushd $ISA_SIM_DIR && cmake CMakeLists.txt && make && popd || exit 1
pushd $PAASM_BUILD_DIR && make paasm && popd || exit 1

#make test directories
mkdir -p $TESTS_ASM_DIR
mkdir -p $TESTS_BIN_DIR
mkdir -p $TESTS_EXPECTED_DIR
mkdir -p $TESTS_EMU_ACTUAL_DIR
mkdir -p $TESTS_SIM_ACTUAL_DIR
mkdir -p $TESTS_HW_ACTUAL_DIR

#generate tests
$ISA_GEN_DIR/ISATestsGen $TESTS_ASM_DIR $TESTS_EXPECTED_DIR || exit 1

tests_fail=0
tests_succeed=0

#compile asm to binary
shopt -s nullglob
for file in $(pwd)/asm-tests/tests/asm/*
do
    tmpfilename=$(basename "$file" .s)
    echo ""
    echo "-- Testing ${tmpfilename} --"
    
    #compile test to bin
    echo -n "Compiling assembly:        "
    if $PAASM_EXE_DIR/paasm "$file" $TESTS_BIN_DIR/$tmpfilename.bin > $LOG_FILE 2>&1 ; 
    then
        echo "Done"
    else
        echo "Error"
        cat $LOG_FILE
        exit 1
    fi
    
    #get isa sim output
    echo -n "Running simulator:         "
    if $ISA_SIM_DIR/ISASim $TESTS_BIN_DIR/$tmpfilename.bin $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart > $LOG_FILE 2>&1 ; 
    then
        echo "Done"
    else
        echo "Error"
        cat $LOG_FILE
        exit 1
    fi
    
    echo -n "Checking simulator result: "
    #compare isa sim output with expected output
    if cmp -s $TESTS_EXPECTED_DIR/$tmpfilename.uart $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart;
    then
        tests_succeed=$(($tests_succeed+1))
        echo "Done"
    else
        tests_fail=$(($tests_fail+1))
        echo "Error"
        echo "expected:"
        #hexdump $TESTS_EXPECTED_DIR/$tmpfilename.uart
        xxd -b -c 4 $TESTS_EXPECTED_DIR/$tmpfilename.uart
        echo "actual:"
        #hexdump $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart
        xxd -b -c 4 $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart
    fi
done
shopt -u nullglob

total_tests=$(($tests_succeed + $tests_fail))
echo "Tests completed: ${total_tests}"
echo "Tests failed:    ${tests_fail}"
echo "Tests Succeeded: ${tests_succeed}"





exit 0