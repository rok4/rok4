#!/bin/bash
echo "test ok conversion"
mergeNtiff -f inputs/conf_conversion.txt -c zip -i lanczos -n 255,255,255,0 -a uint -b 8 -s 4
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi