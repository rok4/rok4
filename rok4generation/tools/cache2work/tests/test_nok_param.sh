#!/bin/bash

echo "test nok param"
cache2work -c jpeg inputs/ORTHOHR.tif outputs/test_nok_param.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi