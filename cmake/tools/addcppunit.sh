#!/bin/bash
## Linux
LSOF=$(lsof -p $$ | grep -E "/"$(basename "$0")"$")
MY_PATH=$(echo "$LSOF" | sed -r s/'^([^\/]+)\/'/'\/'/1 2>/dev/null)
if [ $? -ne 0 ]; then
## OSX
MY_PATH=$(echo "$LSOF" | sed -E s/'^([^\/]+)\/'/'\/'/1 2>/dev/null)
fi

MY_PID=$$
MY_ROOT=$(dirname "$MY_PATH")
MY_NAME=$(basename "$0")

CLASSNAME=CppUnit"$1"
TEMPLATETEST="$MY_ROOT"/../template/tests/cppunit/TemplateUnitTest.cpp

echo -e "Create\t$CLASSNAME"
echo -e "In\t`pwd`"
echo -e "From\t$TEMPLATETEST"

sed -e "s/TEMPLATECLASS/$CLASSNAME/g" "$TEMPLATETEST" > "$CLASSNAME".cpp

