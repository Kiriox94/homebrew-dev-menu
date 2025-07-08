#!/bin/bash
clear

esc=$'\033'

echo "${esc}[43;30mStart compilation${esc}[0m"  # Jaune fond, texte noir
make
exitcode=$?

if [ $exitcode -eq 0 ]; then
    echo "${esc}[42;30mSuccessful compilation${esc}[0m"  # Vert fond, texte noir

    for file in *.nro; do
        if [ -f "$file" ]; then
            echo "${esc}[45;30mSending to switch...${esc}[0m"  # Magenta fond
            nxlink "$file" -r 20
            echo
            exit
        fi
    done
else
    echo "${esc}[41;97mCompilation failed${esc}[0m"  # Rouge fond, texte clair
fi
