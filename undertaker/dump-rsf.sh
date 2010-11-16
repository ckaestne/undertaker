#!/bin/bash

MODELS=${MODELS:-models}
PATH=$PATH:/proj/i4vamos/tools/bin
TMP=$(mktemp)

mkdir -p "$MODELS"

for i in alpha arm avr32 blackfin cris frv h8300 ia64 m32r m68k m68knommu microblaze mips mn10300 parisc powerpc s390 score sh sparc tile x86 xtensa
do
  echo "Calculating model for $i"
  env ARCH="$i" dumpconf arch/$i/Kconfig | grep -v '^#' > $TMP
  rsf2model $TMP > "$MODELS/$i"
done

rm -f $TMP
