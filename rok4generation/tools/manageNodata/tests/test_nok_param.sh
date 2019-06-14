#!/bin/bash

echo "test nok param"
manageNodata -target 255,255,255 -touch-edges -data 254,254 inputs/base.tif outputs/test_nok_param.tif -channels 3 -format uint8 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi