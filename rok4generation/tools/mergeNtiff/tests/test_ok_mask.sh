#!/bin/bash
echo "test ok mask"
mergeNtiff -f inputs/conf_mask.txt -r ./inputs/ -c zip -i lanczos -n 0,0,255
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi