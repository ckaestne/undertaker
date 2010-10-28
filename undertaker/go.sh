#!/bin/bash -e

echo "Running on Linux Version $(git describe)"
find . -name "*.[hcS]" | shuf > worklist
find . -name '*dead' -exec rm -f {} +
echo "Analyzing `wc -l < worklist` files"
time undertaker -t `getconf _NPROCESSORS_ONLN` -b worklist
printf "\n\nFound %s defects\n" `find . -name '*dead'| grep globally | wc -l`
