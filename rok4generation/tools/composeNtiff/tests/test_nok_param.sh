#!/bin/bash

echo "test nok param"
composeNtiff -c jpg -s inputs/ok/ -g 3 3 outputs/test_nok_param.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi