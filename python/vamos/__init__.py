"""
Attributes:

   default_architecture   -- the default architecture to use
   prefer_32bit  -- If running on an amd64 64bit host, prefer configuring and compiling for 32bit.
   kernelversion -- normally determined automatically, but can be overridden here
"""

import os

default_architecture = "x86"

if os.environ.has_key("VAMOS_PREFER_64BIT") or \
        os.environ.has_key('ARCH') and os.environ['ARCH'] == 'x86_64':
    prefer_32bit = False
else:
    prefer_32bit = True

kernelversion = None

