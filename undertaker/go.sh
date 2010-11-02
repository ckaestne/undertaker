#!/bin/bash -e

version="$(git describe)"
if [ -z "$version" ]; then
    echo "Please run in a Linux tree"
else
    echo "Running on Linux Version ${version}"
fi

find . -name "*.[hcS]" | shuf > worklist
find . -name '*dead' -exec rm -f {} +
echo "Analyzing `wc -l < worklist` files"
time undertaker -t `getconf _NPROCESSORS_ONLN` -b worklist
printf "\n\nFound %s global defects\n" `find . -name '*dead'| grep globally | wc -l`
