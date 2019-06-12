#!/bin/bash
echo "test ok orthohr"
cache2work -c zip inputs/ORTHOHR.tif outputs/test_ok_orthohr.tif
if [ $? != 0 ] ; then 
    exit 1
else
    exit 0
fi