#!/bin/bash

# test all spl files in specified directory
# usage: test.sh <directory>

SPLC=./bin/splc

if [ $# -eq 0 ]; then
    echo "usage: test.sh <directory>"
    exit 1
fi

if [ ! -f "$SPLC" ]; then
    echo "cannot find splc at $SPLC"
    exit 1
fi

if [ ! -n "$(ls $1/*.spl 2>/dev/null)" ]; then
    echo "no .spl file in $1"
    exit 0
fi

for i in $1/*.spl; do
    $SPLC $i > ${i%.spl}.ir
    echo "apply splc on $i, return value $?"
done

exit 0