
from distutils.core import setup

with open('VERSION') as f:
    version = f.read().rstrip('\n')

setup (name='vamos',
       version=version,
       package_dir={'vamos.rsf2model' : 'rsf2model/vamos/rsf2model',
	            'vamos' : 'rsf2model/vamos/',},
       packages=['vamos.rsf2model', 'vamos'],
       scripts=['rsf2model/rsf2model'],
       )
