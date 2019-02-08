#!/bin/bash
echo "test ok file full"
pbf2cache -r inputs/pbfs/ -t 3 3 -ultile 258 175 outputs/test_ok_file_full.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi