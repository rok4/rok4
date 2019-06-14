#!/bin/bash

echo "test nok slab"
checkWork inputs/SLAB.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi