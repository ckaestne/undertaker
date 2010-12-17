#!/bin/bash -e

version="$(git describe)"
if [ -z "$version" ]; then
    echo "Please run in a Linux tree"
else
    echo "Running on Linux Version ${version}"
fi

find . -name "*.[hcS]" | shuf > worklist
find . -name '*dead' -exec rm -f {} +

echo "Running coverage analysis"
time undertaker -c -t `getconf _NPROCESSORS_ONLN` -b worklist -m models -M x86 2>/dev/null |
	grep -v '^I:' |
	grep -v '^Ignoring' |
        grep './' > coverage.txt

echo "TOP 50 variable files (format: #possible solutions, filename)"
awk '/^S: / {print $2}' < coverage.txt |
	awk -F, '{ printf "%s %s\n", $3, $1 }' |
	sort -n -r |
	head -n 50 | tee coverage.stats

