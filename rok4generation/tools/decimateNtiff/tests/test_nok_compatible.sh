#!/bin/bash
echo "test nok compatible"
decimateNtiff -f inputs/nok/conf.txt -n 255,0,0 -c zip  2>/dev/null
if [ $? != 0 ] ; then
    rm outputs/test_nok_compatible.tif
    exit 0
else
    rm outputs/test_nok_compatible.tif
    exit 1
fi