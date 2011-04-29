
from distutils.core import setup

with open('VERSION') as f:
    version = f.read().rstrip('\n')

setup (name='undertaker',
       version=version,
       package_dir={'undertaker' : 'rsf2model/undertaker'},
       packages=['undertaker'],
       scripts=['rsf2model/rsf2model'],
       )
