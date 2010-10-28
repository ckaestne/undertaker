#!/bin/bash -e

echo "Running on Linux Version $(git describe)"
find . -name "*.[hcS]" | shuf > worklist
echo "Analyzing `wc -l < worklist` files"
time undertaker -t `getconf _NPROCESSORS_ONLN` -b worklist
printf "Found %s defects\n" `find . -name '*dead'`
