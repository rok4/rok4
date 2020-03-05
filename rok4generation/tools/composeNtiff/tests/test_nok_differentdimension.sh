#!/bin/bash

echo "test nok differentdimension"
composeNtiff -c jpg -s inputs/nok/ -g 2 2 outputs/test_nok_differentdimension.tif 1>/dev/null 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi