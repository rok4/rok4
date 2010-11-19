#!/bin/bash

# building and testing
cd lib
echo "\n\n\n\n***  BUILD LIBS   *****"
make
if [ $? -ne 0 ] ; then
  exit 1
fi

cd be4
echo "\n\n\n\n***  BUILD BE4   *****"
make
if [ $? -ne 0 ] ; then
  exit 2
fi
