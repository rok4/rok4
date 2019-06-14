#!/bin/bash
echo "test ok scan1000"
composeNtiff -c zip -s inputs/ok/ -g 2 2 outputs/test_ok_scan1000.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi