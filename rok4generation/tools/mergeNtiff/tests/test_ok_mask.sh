#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test ok mask"
$precommand mergeNtiff -f inputs/conf_mask.txt -r ./inputs/ -c zip -i lanczos -n 0,0,255
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