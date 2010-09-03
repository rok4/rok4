#!/bin/bash

# building and testing
cd lib
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make -C libimage very-clean test
if [ $? -ne 0 ] ; then
  exit 1
fi


# documentation
# echo "\n\n\n\n***  DOCUMENTATION    *****"
# make doc
# if [ $? -ne 0 ] ; then
#   exit 2
# fi


