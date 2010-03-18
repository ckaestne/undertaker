#!/bin/sh

RSFS=`find . -name Kconfig`
WHAT=`cat $1`

for j in $WHAT
do
	echo $j
	SEARCH=`echo $j | sed -e 's,^CONFIG_,,g'`
	for i in $RSFS
	do
		ERG="`grep $SEARCH $i`"
		[ -z "$ERG" ] || echo $i | sed -e 's,/Kconfig$,,g'
	done
done