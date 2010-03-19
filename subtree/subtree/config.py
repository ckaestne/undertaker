#!/usr/bin/python

import tag.info

class Config:
    def __init__(self, name, tags):
        self.name = name
        self.tags = tags[name]

    def _tags(self):
        return ['<span title="%s">%s</span>' % (tag.info.entry[i], i)
                for i in self.tags]

    def __str__(self):
        return '%s (%s)' % ( self.name, ", ".join(self._tags()) )

    def __hash__(self):
        return self.name.__hash__()
