#!/bin/bash
valgrind=$1

precommand=""
if [ $valgrind == "1" ] ; then 
    precommand="valgrind --log-file=$0_valgrind.log --show-reachable=no"
fi

echo "test nok param"
$precommand overlayNtiff -f inputs/conf.txt -m ALPHATOP -s 2 -c zip -p rgb -t 255,255,255 -b 255 1>/dev/null 2>/dev/null
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