#!/bin/bash

ISA_GEN_DIR=$(pwd)/asm-tests/ISATestsGen/ISATestsGen
ISA_SIM_DIR=$(pwd)/asm-tests/ISASim/ISASim
PAASM_BUILD_DIR=$(pwd)/simulator/build
PAASM_EXE_DIR=$PAASM_BUILD_DIR/src

TESTS_ASM_DIR=$(pwd)/asm-tests/tests/asm
TESTS_BIN_DIR=$(pwd)/asm-tests/tests/bin
TESTS_EXPECTED_DIR=$(pwd)/asm-tests/tests/expected
TESTS_EMU_ACTUAL_DIR=$(pwd)/asm-tests/tests/emu-actual
TESTS_ISA_ACTUAL_DIR=$(pwd)/asm-tests/tests/isa-actual
TESTS_HW_ACTUAL_DIR=$(pwd)/asm-tests/tests/hw-actual


#compile programs
pushd $ISA_GEN_DIR && cmake CMakeLists.txt && make && popd
pushd $ISA_SIM_DIR && cmake CMakeLists.txt && make && popd
pushd $PAASM_BUILD_DIR && make paasm && popd

#make test directories
mkdir -p $TESTS_ASM_DIR
mkdir -p $TESTS_BIN_DIR
mkdir -p $TESTS_EXPECTED_DIR
mkdir -p $TESTS_EMU_ACTUAL_DIR
mkdir -p $TESTS_ISA_ACTUAL_DIR
mkdir -p $TESTS_HW_ACTUAL_DIR

#generate tests
$ISA_GEN_DIR/ISATestsGen $TESTS_ASM_DIR $TESTS_EXPECTED_DIR || exit 1

#compile asm to binary
shopt -s nullglob
for file in $(pwd)/asm-tests/tests/asm/*
do
    echo "$file"
    tmpfilename=$(basename "$file" .s)
    $PAASM_EXE_DIR/paasm "$file" $TESTS_BIN_DIR/$tmpfilename.bin || exit 1
done
shopt -u nullglob





exit 0
