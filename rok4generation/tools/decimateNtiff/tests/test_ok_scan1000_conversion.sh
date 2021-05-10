#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test ok scan1000 conversion"
$precommand decimateNtiff -f inputs/ok/conf.txt -n 255,0,0 -c zip -a uint -b 8 -s 1
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