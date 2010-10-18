#!/bin/bash

# building and testing
cd be4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make
if [ $? -ne 0 ] ; then
  exit 1
fi
