#!/bin/bash
echo "test ok alphatop"
overlayNtiff -f inputs/conf.txt -m ALPHATOP -s 4 -c zip -p rgb -t 255,255,255 -b 255,0,0,100
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi