#!/bin/bash
echo "test ok scan1000 bg"
decimateNtiff -f inputs/ok/conf_bg.txt -n 255,0,0 -c zip
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi