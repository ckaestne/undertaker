
from distutils.core import setup

with open('VERSION') as f:
    version = f.read().rstrip('\n')

setup (name='vamos',
       version=version,
       package_dir={'vamos' : 'python/vamos/',
                    'vamos.rsf2model' : 'python/vamos/rsf2model',
                    },
       packages=['vamos',
                 'vamos.rsf2model',
                 ],
       scripts=['python/rsf2model',
                ],
       )
