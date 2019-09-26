#!/bin/bash
echo "test ok crop"
work2cache inputs/NOTASLAB.tif -c jpg -t 100 100 outputs/test_ok_crop.tif -crop
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi
