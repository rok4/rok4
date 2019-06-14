#!/bin/bash
echo "test nok param"
overlayNtiff -f inputs/conf.txt -m ALPHATOP -s 2 -c zip -p rgb -t 255,255,255 -b 255 2>/dev/null
if [ $? != 0 ] ; then 
    exit 0
else
    exit 1
fi