#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test nok not a slab"
$precommand cache2work -c zip inputs/NOTASLAB.tif outputs/test_nok_notaslab.tif 1>/dev/null 2>/dev/null
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