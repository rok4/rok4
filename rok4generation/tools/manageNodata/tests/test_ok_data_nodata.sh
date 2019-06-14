#!/bin/bash
echo "test ok mask"
manageNodata -target 255,255,255 -touch-edges -data 255,0,0 -nodata 200,200,255 inputs/base.png outputs/test_ok_data_nodata.tif -channels 3 -format uint8
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi