#!/usr/bin/python

class Config:
    def __init__(self, name, tags):
        self.name = name
        self.tags = tags[name]

    def __str__(self):
        return '%s (%s)' % ( self.name, ", ".join(self.tags) )

    def __hash__(self):
        return self.name.__hash__()
