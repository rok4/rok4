#!/bin/bash
echo "test ok file hole"
pbf2cache -r inputs/pbfs/ -t 5 5 -ultile 257 174 outputs/test_ok_file_hole.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi