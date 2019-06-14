#!/bin/bash
echo "test nok param"
work2cache inputs/NOTASLAB.tif -c zip -t 150 150 outputs/test_nok_param.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi