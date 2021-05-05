#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test ok file full"
$precommand pbf2cache -r inputs/pbfs/ -t 3 3 -ultile 258 175 outputs/test_ok_file_full.tif
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