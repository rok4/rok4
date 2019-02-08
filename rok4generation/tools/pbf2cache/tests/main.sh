#!/bin/bash
TOOL="PBF2CACHE"

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")

tests=( $( ls $BASEDIR/test_*.sh ) )
tests_nb=${#tests[*]}

i=0
errors=0
while [ $i -lt $tests_nb ]; do
    let num=$i+1
    echo "Test $num/$tests_nb"
    bash ${tests[$i]}
    if [ $? != 0 ] ; then 
        let errors=$errors+1
        echo "    -> NOK"
    else
        echo "    -> OK"
    fi
    let i++
done

if [ $errors != 0 ] ; then 
    echo "$TOOL tested with error(s) ($errors / $tests_nb)"
    exit 1
else
    echo "$TOOL tested without error"
    echo 0
fi