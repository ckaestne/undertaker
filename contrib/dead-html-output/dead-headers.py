#!/usr/bin/env python

import fnmatch
import os
import sys

import json

matches = []

result = {}
for parent, dirnames, filenames in os.walk('.'):
  for fn in fnmatch.filter(filenames, '*globally*'):
      filename = os.path.join(parent, fn)

      fd = open(filename)
      header = fd.readline().split(":")
      linecount = [header[2], header[5]]
      fd.close()

      path = filename.split("/")
      root = result
      for item in range(1, len(path) - 1):
          if not path[item] in root:
              root[path[item]] = {}
          root = root[path[item]]

      root[path[-1]] = linecount
      matches.append(filename)

print json.dumps(result)

sys.stderr.write("%d deads\n" % len(matches))
