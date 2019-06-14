#!/bin/bash

echo "test nok param"
merge4tiff -c zip -n 255,255,255 -i1 inputs/01.jpg -i2 inputs/02.jpg -i3 inputs/03.jpg -m3 inputs/03.tif -a uint -b 8 -s 4 -io outputs/test_nok_param.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi