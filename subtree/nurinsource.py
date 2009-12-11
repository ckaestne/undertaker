#!/usr/bin/python

filename="some-results-for-nur-in-source"

f = file(filename).readlines()

cur = ""
qdict = dict()

for i in f:
    if i[:6] == "CONFIG":
        cur = i[:-1]
        qdict[i[:-1]] = []
    else:
        qdict[cur].append(i[2:-1])

def gqdict():
    return qdict
