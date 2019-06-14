#!/bin/bash

echo "test nok inputs"
mergeNtiff -f inputs/conf_nok_inputs.txt -r ./inputs/ -c zip -i lanczos -n 0,0,255  2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi