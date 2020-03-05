#!/bin/bash
echo "test nok param"
decimateNtiff -f inputs/ok/conf.txt -n 255,0 -c zip  1>/dev/null 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi