#!/bin/sh -e

PATH=$PATH:/proj/i4vamos/tools/bin

for i in alpha arm avr32 blackfin cris frv h8300 ia64 m32r m68k m68knommu microblaze mips mn10300 parisc powerpc s390 score sh sparc tile x86 xtensa
do
  echo $i;
  env ARCH="$i" dumpconf arch/$i/Kconfig | grep -v '^#' > kconfig-$i.rsf
done
