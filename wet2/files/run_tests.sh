#!/bin/sh
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

make all
make test

GRADE=$(cat grade.txt)
NUM_TESTS=$(find . -type f -name "test*.cxx" | wc -l)
make clean

if [ $GRADE -eq $NUM_TESTS ] ; then
    echo "${GREEN}PASSED${NC}"
else
    echo "${RED}FAILED${NC}"
fi