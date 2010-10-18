#!/bin/bash

# building and testing
cd libs
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make
if [ $? -ne 0 ] ; then
  exit 1
fi

cd ../be4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make
if [ $? -ne 0 ] ; then
  exit 2
fi
