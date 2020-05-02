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
pushd $ISA_GEN_DIR && cmake CMakeLists.txt && make && popd
pushd $ISA_SIM_DIR && cmake CMakeLists.txt && make && popd
pushd $PAASM_BUILD_DIR && make paasm && popd

#make test directories
mkdir -p $TESTS_ASM_DIR
mkdir -p $TESTS_BIN_DIR
mkdir -p $TESTS_EXPECTED_DIR
mkdir -p $TESTS_EMU_ACTUAL_DIR
mkdir -p $TESTS_SIM_ACTUAL_DIR
mkdir -p $TESTS_HW_ACTUAL_DIR

#generate tests
$ISA_GEN_DIR/ISATestsGen $TESTS_ASM_DIR $TESTS_EXPECTED_DIR || exit 1

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
        echo "Done"
    else
        echo "Error"
        echo "expected:"
        hexdump $TESTS_EXPECTED_DIR/$tmpfilename.uart
        echo "actual:"
        hexdump $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart
    fi
done
shopt -u nullglob





exit 0
