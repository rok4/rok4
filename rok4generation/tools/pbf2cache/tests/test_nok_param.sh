#!/bin/bash

echo "test nok param"
pbf2cache -r inputs/pbfs/ -t 3 -ultile 258 175 outputs/test_nok_param.tif 1>/dev/null 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi