#!/bin/bash
echo "test ok mask"
manageNodata -target 255,255,255 inputs/base.png -mask-out outputs/test_ok_mask.tif -channels 3 -format uint8
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi