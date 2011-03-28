#!/bin/sh
help2man -N -h -h -v -v -n "find defects in conditinal C code" \
  -i help2man-add ../undertaker/undertaker \
  | sed 's#^\\- *\(.*\): *\(.*\)$#.br\n.B \1\n \2#' > undertaker.1

help2man -N -h -h -v -v -n "run undertaker on linux tree"\
  -i help2man-add ../undertaker/undertaker-linux-tree \
  > undertaker-linux-tree.1
