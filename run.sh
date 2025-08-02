#!/bin/bash

NC='\033[0m'

echo "\033[43;30mStart compilation${NC}"
make
exitcode=$?

if [ $exitcode -eq 0 ]; then
    echo "\033[42;30mSuccessful compilation${NC}"
    echo "\033[45;30mSending the nro to the switch...${NC}"

    nxlink dev-menu.nro -r 20
    echo
else
    echo "\033[41;97mCompilation failed${NC}" 
fi