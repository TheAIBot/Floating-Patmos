#!/bin/bash

TESTS_DIR=$(pwd)/asm-tests
ISA_GEN_DIR=$TESTS_DIR/ISATestsGen
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
TESTS_LOGS_DIR=$TESTS_DIR/tests/logs

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
mkdir -p $TESTS_LOGS_DIR

#generate tests
$ISA_GEN_DIR/ISATestsGen/ISATestsGen $TESTS_ASM_DIR $TESTS_EXPECTED_DIR || exit 1

echo -n "Compiling emulator "
#cp $TESTS_DIR/uartForTests.s $(pwd)/asm/uartForTests.s
#if make emulator BOOTAPP=uartForTests;# > $LOG_FILE 2>&1 ; 
#then
#    echo "Done"
#else
#    echo "Error"
#    cat $LOG_FILE
#    exit 1
#fi
echo ""

tests_fail=0
tests_succeed=0

shopt -s nullglob
for test_dir in `find $TESTS_ASM_DIR/ -type d -links 2`
do
    #echo $test_dir
    test_name=${test_dir#$TESTS_ASM_DIR/}
    printf "Testing %-40s [" $test_name
    for file in $test_dir/*
    do
        tmpfilename=$(basename "$file" .s)
        
        #mkdir -p $TESTS_ASM_DIR/$test_name
        mkdir -p $TESTS_BIN_DIR/$test_name
        #mkdir -p $TESTS_EXPECTED_DIR/$test_name
        mkdir -p $TESTS_EMU_ACTUAL_DIR/$test_name
        mkdir -p $TESTS_SIM_ACTUAL_DIR/$test_name
        mkdir -p $TESTS_HW_ACTUAL_DIR/$test_name
        mkdir -p $TESTS_LOGS_DIR/$test_name
        
        logfile=$TESTS_LOGS_DIR/$test_name/$(basename "$file" .s).log
        #echo ""
        #echo -n "Testing \t ${tmpfilename} ["
        #printf  "Testing %8s [" $tmpfilename
        
        #compile test to bin
        #echo -n "Compiling assembly:        "
        if $PAASM_EXE_DIR/paasm "$file" $TESTS_BIN_DIR/$test_name/$tmpfilename.bin > $logfile 2>&1 ; 
        then
            #echo -n "-"
            touch $TESTS_BIN_DIR/$test_name/$tmpfilename.dat
        else
            #echo " Failed to compile assembly"
            echo -n "C"
            tests_fail=$(($tests_fail+1))
            continue
        fi
        
        #get isa sim output
        #echo -n "Running simulator:         "
        if $ISA_SIM_DIR/ISASim $TESTS_BIN_DIR/$test_name/$tmpfilename.bin $TESTS_SIM_ACTUAL_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1 ; 
        then
            #echo -n "-"
            echo -n ""
        else
            #echo " Failed to simulate binary"
            echo -n "S"
            tests_fail=$(($tests_fail+1))
            continue
        fi
        
        #echo -n "Checking simulator result: "
        #compare isa sim output with expected output
        if cmp -s $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart $TESTS_SIM_ACTUAL_DIR/$test_name/$tmpfilename.uart;
        then
            #echo -n "-"
            echo -n ""
        else
            tests_fail=$(($tests_fail+1))
            #echo " Simulator gave an incorrect result"
            echo -n "S"
            
            echo "expected:" >> $logfile
            xxd -b -c 4 $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            echo "actual:" >> $logfile
            xxd -b -c 4 $TESTS_SIM_ACTUAL_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            echo "diff:" >> $logfile
            cmp -l $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart $TESTS_SIM_ACTUAL_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            continue
        fi
        
        $EMU_EXE_DIR/emulator -i -l 2000000 -O $TESTS_EMU_ACTUAL_DIR/$test_name/$tmpfilename.uart -b $TESTS_BIN_DIR/$test_name/$tmpfilename.bin >> $logfile 2>&1
        #echo -n "-"
        
        if cmp -s $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart $TESTS_EMU_ACTUAL_DIR/$test_name/$tmpfilename.uart;
        then
            tests_succeed=$(($tests_succeed+1))
            rm $logfile
            echo -n "-"
        else
            tests_fail=$(($tests_fail+1))
            #echo " Emulator gave an incorrect result"
            echo -n "E"
            
            echo "expected:" >> $logfile
            xxd -b -c 4 $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            echo "actual:" >> $logfile
            xxd -b -c 4 $TESTS_EMU_ACTUAL_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            echo "diff:" >> $logfile
            cmp -l $TESTS_EXPECTED_DIR/$test_name/$tmpfilename.uart $TESTS_EMU_ACTUAL_DIR/$test_name/$tmpfilename.uart >> $logfile 2>&1
            continue
        fi
    done
    echo "]"
done
shopt -u nullglob

total_tests=$(($tests_succeed + $tests_fail))
echo "Tests completed: ${total_tests}"
echo "Tests failed:    ${tests_fail}"
echo "Tests Succeeded: ${tests_succeed}"





exit 0
