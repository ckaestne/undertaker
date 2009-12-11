#!/bin/sh

RSFS=`find . -name '*.c' -o -name '*.h'`
WHAT=`cat $i`

for j in $WHAT
do
	echo $j
	for i in $RSFS
	do
		ERG="`grep $j $i`"
		[ -z "$ERG" ] || echo $i
	done
done