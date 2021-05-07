#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test ok mask"
$precommand manageNodata -target 255,255,255 -touch-edges -data 255,0,0 -nodata 200,200,255 inputs/base.png outputs/test_ok_data_nodata.tif -channels 3 -format uint8
ret=$?
if [ $valgrind == "1" ] ; then 
    grep " lost: " $0_valgrind.log
    rm $0_valgrind.log
fi

if [ $ret != 0 ] ; then 
    exit 1
else
    exit 0
fi