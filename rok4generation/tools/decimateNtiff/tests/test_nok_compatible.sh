#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test nok compatible"
$precommand decimateNtiff -f inputs/nok/conf.txt -n 255,0,0 -c zip  1>/dev/null 2>/dev/null
ret=$?
if [ $valgrind == "1" ] ; then 
    grep " lost: " $0_valgrind.log
    rm $0_valgrind.log
fi

if [ $ret != 0 ] ; then 
    rm outputs/test_nok_compatible.tif
    exit 0
else
    rm outputs/test_nok_compatible.tif
    exit 1
fi