#!/bin/bash

set -e

MODELS=${MODELS:-models}
PATH=$PATH:/proj/i4vamos/tools/bin
TMP=$(mktemp)

if [ ! -f arch/x86/Kconfig ]; then
    echo "Not run in an Linux Tree"
    exit 1
fi

ARCHS=$(ls arch/*/Kconfig | cut -d '/' -f 2)

mkdir -p "$MODELS"

for i in $ARCHS
do
  echo "Calculating model for $i"
  env ARCH="$i" dumpconf arch/$i/Kconfig | grep -v '^#' > "$MODELS/kconfig-$i.rsf"
  rsf2model "$MODELS/kconfig-$i.rsf" > "$MODELS/$i.model"

  # Add CONFIG_$i -> !CONFIG_*
  UPCASE_ARCH=$(echo $i | tr 'a-z' 'A-Z')
  echo $ARCHS | sed "s/ *$i */ /" | tr 'a-z' 'A-Z' | sed 's/$/"/; s/\</!CONFIG_/g; s/ / \&\& /g; s/^/'"CONFIG_$UPCASE_ARCH "'"/;' >> "$MODELS/$i.model"
done
