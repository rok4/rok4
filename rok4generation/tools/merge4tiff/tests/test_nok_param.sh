#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi


echo "test nok param"
$precommand merge4tiff -c zip -n 255,255,255 -i1 inputs/01.jpg -i2 inputs/02.jpg -i3 inputs/03.jpg -m3 inputs/03.tif -a uint -b 8 -s 4 -io outputs/test_nok_param.tif 1>/dev/null 2>/dev/null
ret=$?
if [ $valgrind == "1" ] ; then 
    grep " lost: " $0_valgrind.log
    rm $0_valgrind.log
fi

if [ $ret != 0 ] ; then 
    exit 0
else
    exit 1
fi