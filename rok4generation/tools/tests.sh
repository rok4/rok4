#!/bin/bash

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")

valgrind="0"
precommand=""

ARGUMENTS="valgrind,help"
# read arguments
opts=$(getopt \
    --longoptions "${ARGUMENTS}" \
    --name "$(basename "$0")" \
    --options "" \
    -- "$@"
)
eval set --$opts

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help)
            echo "./tests.sh <OPTIONS>"
            echo "    --help"
            echo "    --valgrind"
            exit 0
            ;;

        --valgrind)
            valgrind=1
            echo "Commands executed with valgrind analysis"
            shift 1
            ;;

        *)
            break
            ;;
    esac
done


tests=( $( find $BASEDIR -name "main.sh" ) )
tests_nb=${#tests[*]}

i=0
errors=0
while [ $i -lt $tests_nb ]; do
    let num=$i+1
    test_basedir=$(dirname "${tests[$i]}")
    cd $test_basedir

    bash ${tests[$i]} $valgrind
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