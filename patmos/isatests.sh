#!/bin/bash

TESTS_DIR=$(pwd)/asm-tests
ISA_GEN_DIR=$TESTS_DIR/ISATestsGen/ISATestsGen
ISA_SIM_DIR=$TESTS_DIR/ISASim/ISASim
PAASM_BUILD_DIR=$(pwd)/simulator/build
PAASM_EXE_DIR=$PAASM_BUILD_DIR/src
EMU_EXE_DIR=$(pwd)/hardware/build

TESTS_ASM_DIR=$TESTS_DIR/tests/asm
TESTS_BIN_DIR=$TESTS_DIR/tests/bin
TESTS_EXPECTED_DIR=$TESTS_DIR/tests/expected
TESTS_EMU_ACTUAL_DIR=$TESTS_DIR/tests/emu-actual
TESTS_SIM_ACTUAL_DIR=$TESTS_DIR/tests/isa-actual
TESTS_HW_ACTUAL_DIR=$TESTS_DIR/tests/hw-actual

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

echo -n "Compiling emulator "
cp $TESTS_DIR/uartForTests.s $(pwd)/asm/uartForTests.s
if make emulator BOOTAPP=uartForTests;# > $LOG_FILE 2>&1 ; 
then
    echo "Done"
else
    echo "Error"
    cat $LOG_FILE
    exit 1
fi

tests_fail=0
tests_succeed=0

#compile asm to binary
shopt -s nullglob
for file in $TESTS_ASM_DIR/*
do
    tmpfilename=$(basename "$file" .s)
    #echo ""
    #echo -n "Testing \t ${tmpfilename} ["
    printf  "Testing %8s [" $tmpfilename
    
    #compile test to bin
    #echo -n "Compiling assembly:        "
    if $PAASM_EXE_DIR/paasm "$file" $TESTS_BIN_DIR/$tmpfilename.bin > $LOG_FILE 2>&1 ; 
    then
        echo -n "-"
        touch $TESTS_BIN_DIR/$tmpfilename.dat
    else
        echo "Compile error"
        cat $LOG_FILE
        tests_fail=$(($tests_fail+1))
        continue
    fi
    
    #get isa sim output
    #echo -n "Running simulator:         "
    if $ISA_SIM_DIR/ISASim $TESTS_BIN_DIR/$tmpfilename.bin $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart > $LOG_FILE 2>&1 ; 
    then
        echo -n "-"
    else
        echo "Simulation error"
        cat $LOG_FILE
        tests_fail=$(($tests_fail+1))
        continue
    fi
    
    #echo -n "Checking simulator result: "
    #compare isa sim output with expected output
    if cmp -s $TESTS_EXPECTED_DIR/$tmpfilename.uart $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart;
    then
        echo -n "-"
    else
        tests_fail=$(($tests_fail+1))
        echo "Error"
        #echo "expected:"
        #xxd -b -c 4 $TESTS_EXPECTED_DIR/$tmpfilename.uart
        #echo "actual:"
        #xxd -b -c 4 $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart
        #echo "diff:"
        #cmp -l $TESTS_EXPECTED_DIR/$tmpfilename.uart $TESTS_SIM_ACTUAL_DIR/$tmpfilename.uart
        continue
    fi
    
    cp $file $(pwd)/asm/$tmpfilename.s || exit 1
    make asm BOOTAPP=$tmpfilename > /dev/null 2>&1
    #$PAASM_EXE_DIR/paasm $file $(pwd)/tmp/$tmpfilename.bin > /dev/null 2>&1
	#touch $(pwd)/tmp/$tmpfilename.dat

    
    
    #pushd hardware
    #if make emulator BOOTBIN=$TESTS_BIN_DIR/$tmpfilename.bin > $LOG_FILE 2>&1 ; 
    #then
    #    popd
    #    echo -n "-"
    #else
    #    popd
    #    echo "Compile emulator error"
    #    cat $LOG_FILE
    #    tests_fail=$(($tests_fail+1))
    #    continue
    #fi
    
    $EMU_EXE_DIR/emulator -i -l 1000000 -O $TESTS_EMU_ACTUAL_DIR/$tmpfilename.uart -b $(pwd)/tmp/$tmpfilename.bin > /dev/null
    echo -n "-"
    
    if cmp -s $TESTS_EXPECTED_DIR/$tmpfilename.uart $TESTS_EMU_ACTUAL_DIR/$tmpfilename.uart;
    then
        tests_succeed=$(($tests_succeed+1))
        echo "-]"
    else
        tests_fail=$(($tests_fail+1))
        echo "Error"
        #echo "expected:"
        #xxd -b -c 4 $TESTS_EXPECTED_DIR/$tmpfilename.uart
        #echo "actual:"
        #xxd -b -c 4 $TESTS_EMU_ACTUAL_DIR/$tmpfilename.uart
        #echo "diff:"
        #cmp -l $TESTS_EXPECTED_DIR/$tmpfilename.uart $TESTS_EMU_ACTUAL_DIR/$tmpfilename.uart
        continue
    fi
done
shopt -u nullglob

total_tests=$(($tests_succeed + $tests_fail))
echo "Tests completed: ${total_tests}"
echo "Tests failed:    ${tests_fail}"
echo "Tests Succeeded: ${tests_succeed}"





exit 0
