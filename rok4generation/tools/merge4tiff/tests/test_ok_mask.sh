#!/bin/bash
echo "test ok mask"
merge4tiff -c zip -n 0,255,0 -i1 inputs/01.jpg -i2 inputs/02.jpg -i3 inputs/03.jpg -m3 inputs/03m.tif -io outputs/test_ok_mask_i.tif -mo outputs/test_ok_mask_m.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi