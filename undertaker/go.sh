#!/bin/bash

UNDERTAKER=/proj/i4vamos/users/sincero/vamos/undertaker/undertaker
PREFIX=worklist
RSFSET=work.txt
LINES=`cat $RSFSET | wc -l`
CORES=6
NF=$(expr $LINES / $CORES)

echo $LINES
echo $CORES
echo $NF

split -d -l $NF $RSFSET $PREFIX-

for i in $PREFIX-*
do
  echo $i; 
  (time $UNDERTAKER -b $i 2> $i.err > $i.out)&
#  (time $UNDERTAKER -s -b $i 2> $i.err > $i.out)&
done

