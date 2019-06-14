#!/bin/bash
echo "test ok scan1000 conversion"
decimateNtiff -f inputs/ok/conf.txt -n 255,0,0 -c zip -a uint -b 8 -s 1
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi