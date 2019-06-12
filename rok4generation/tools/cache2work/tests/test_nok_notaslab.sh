#!/bin/bash

echo "test nok not a slab"
cache2work -c zip inputs/NOTASLAB.tif outputs/test_nok_notaslab.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi