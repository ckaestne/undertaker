#!/usr/bin/python

import tag.info

html = '''
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
\t"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="de" lang="de">
<head>
<style type="text/css">
.point {
font-size: 0.5em;
}
</style>

</head>
<body>
<ul>
%s
</ul>
</body>
</html>'''


tagdict = dict()
tags = set()

def init_dicts():
    global tagdict
    global tags
    splitlines = [ i[:-1].split()
                   for i in file('tags').readlines() ]

    tagdict  = dict([ (i[0], i[1:]
                       if len(i[1:])
                       else (''))
                      for i in splitlines ])

    tags = set((''))

    for i in tagdict.values():
        tags |= set(i)

init_dicts()

def tagstring(item):
    return ', '.join(['<span title="%s">%s</span>'
                      % (tag.info.entry[i], i)
                      for i in tagdict[item]
                      ])

def superset(str):
    l = str.split('+')
    return set(l) | set(['%s?' % (i) for  i in l])

def wanted_not(config, ignored):
    return len(ignored.intersection(set(tagdict[config]))) == 0

def wanted_only(config, ignored):
    return len(ignored.intersection(set(tagdict[config]))) != 0

def serve(what, fn):
    c = filter(lambda x: fn(x, what) , tagdict.keys())

    return [ '%s (%s)' % (d, tagstring(d)) for d in c]

def serve_stuff(environment, start_response):
    info = environment.get('PATH_INFO', '').split('/')

    e = set()
    if len(info) == 0:
        e = serve(set())
    else:
        if info[1] == 'not':
            e = serve(superset(info[2]), wanted_not)
        elif info[1] == 'only':
            e = serve(superset(info[2]), wanted_only)
        elif info[1] == 'reload':
            init_dicts()

            start_response('301 Moved Permanently', [('Location', '/not/')])
            return '';
        else:
            start_response('400 NOT FOUND', [])
            return ''

    start_response('200 OK', [('Content-Type', 'application/xhtml+xml')])

    return html % ('\n'.join(sorted( [ '  <li>%s</li>' % i for i in list(e) ] )))

if __name__ == '__main__':
    from wsgiref.simple_server import make_server
    httpd = make_server("", 8000, serve_stuff)
    httpd.serve_forever()
