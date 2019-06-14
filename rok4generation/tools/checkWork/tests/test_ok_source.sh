#!/bin/bash
echo "test ok source"
checkWork inputs/SOURCE.tif 2>/dev/null
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi