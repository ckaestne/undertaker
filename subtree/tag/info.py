#!/usr/bin/python

import sys
import re

class Entries:
    _entry = {'fixed'   : 'A fix was accepted in a Linux tree',
              'sent'    : 'A mail was sent out to get this fixed',
              'define'  : 'This item is #define-ed somewhere',
              'forever' : 'This has been in Linux since git history started (2.6.12-rc)',
              'pre-2.4' : 'This Item was there as bitkeeper History started (2.4)',
              'typo'    : 'This item is caused by a misstyped name',
              'wip'     : 'This item is currently under construction',
              'neu'     : 'This item appeared just recently in linux',
              'obsolete': 'This item was marked deprecated and later removed from linux'
              }

    def __getitem__(self, entry):
        commit_re = re.compile('[0-9a-f]{40}')

        if commit_re.match(entry):
            return 'Take a look at git commit %s for further information' % entry

        if entry[-1:] == '?':
            search = entry[:-1]
        else:
            search = entry

        if search in self._entry:
            return self._entry[search]
        else:
            sys.stderr.write('tag without help -> %s\n' % search)
            return 'No information available'

entry = Entries()
