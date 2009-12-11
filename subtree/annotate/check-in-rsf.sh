#!/bin/sh

RSFS=`find . -name '*.c.rsf'`
WHAT=`cat $1`

echo $i

for j in $WHAT
do
	echo $j
	for i in $RSFS
	do
		ERG="`grep $j $i`"
		[ -z "$ERG" ] || echo $i
	done
done