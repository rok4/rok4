#!/bin/bash

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")

tests=( $( find $BASEDIR -name "main.sh" ) )
tests_nb=${#tests[*]}

i=0
errors=0
while [ $i -lt $tests_nb ]; do
    let num=$i+1
    test_basedir=$(dirname "${tests[$i]}")
    cd $test_basedir

    bash ${tests[$i]}
    if [ $? != 0 ] ; then 
        let errors=$errors+1
    fi
    let i++
    cd $BASEDIR
done

if [ $errors != 0 ] ; then 
    echo "ROK4GENERATION commands tested with error(s) ($errors / $tests_nb)"
    exit 1
else
    echo "ROK4GENERATION commands tested without error"
    exit 0
fi