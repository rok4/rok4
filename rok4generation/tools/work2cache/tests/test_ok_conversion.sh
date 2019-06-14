#!/bin/bash
echo "test ok conversion"
work2cache inputs/NOTASLAB.tif -c zip -t 50 50 -a uint -b 8 -s 1 outputs/test_ok_conversion.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi