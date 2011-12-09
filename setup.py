
from distutils.core import setup

with open('VERSION') as f:
    version = f.read().rstrip('\n')

setup (name='vamos',
       version=version,
       package_dir={'vamos': 'python/vamos'},
       packages=['vamos',
                 'vamos.rsf2model',
                 ],
       scripts=['python/rsf2model',
                ],
       )
