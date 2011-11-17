#!/usr/bin/env python

import glob, os, subprocess, sys

format_dead = "#line%(linum)s { background-color: #ff6655; cursor: pointer;}\n"
format_undead = "#line%(linum)s { background-color: #66dd11; cursor: pointer;}\n"

icon_file_css = """a.icon {
            padding-left: 1.5em;
            text-decoration: none;
     }
a.icon:hover {
     text-decoration: underline;
}
a.file {
            background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABnRSTlMAAAAAAABupgeRAAABHUlEQVR42o2RMW7DIBiF3498iHRJD5JKHurL+CRVBp+i2T16tTynF2gO0KSb5ZrBBl4HHDBuK/WXACH4eO9/CAAAbdvijzLGNE1TVZXfZuHg6XCAQESAZXbOKaXO57eiKG6ft9PrKQIkCQqFoIiQFBGlFIB5nvM8t9aOX2Nd18oDzjnPgCDpn/BH4zh2XZdlWVmWiUK4IgCBoFMUz9eP6zRN75cLgEQhcmTQIbl72O0f9865qLAAsURAAgKBJKEtgLXWvyjLuFsThCSstb8rBCaAQhDYWgIZ7myM+TUBjDHrHlZcbMYYk34cN0YSLcgS+wL0fe9TXDMbY33fR2AYBvyQ8L0Gk8MwREBrTfKe4TpTzwhArXWi8HI84h/1DfwI5mhxJamFAAAAAElFTkSuQmCC ") left top no-repeat;
          }
a.dir {
            background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAd5JREFUeNqMU79rFUEQ/vbuodFEEkzAImBpkUabFP4ldpaJhZXYm/RiZWsv/hkWFglBUyTIgyAIIfgIRjHv3r39MePM7N3LcbxAFvZ2b2bn22/mm3XMjF+HL3YW7q28YSIw8mBKoBihhhgCsoORot9d3/ywg3YowMXwNde/PzGnk2vn6PitrT+/PGeNaecg4+qNY3D43vy16A5wDDd4Aqg/ngmrjl/GoN0U5V1QquHQG3q+TPDVhVwyBffcmQGJmSVfyZk7R3SngI4JKfwDJ2+05zIg8gbiereTZRHhJ5KCMOwDFLjhoBTn2g0ghagfKeIYJDPFyibJVBtTREwq60SpYvh5++PpwatHsxSm9QRLSQpEVSd7/TYJUb49TX7gztpjjEffnoVw66+Ytovs14Yp7HaKmUXeX9rKUoMoLNW3srqI5fWn8JejrVkK0QcrkFLOgS39yoKUQe292WJ1guUHG8K2o8K00oO1BTvXoW4yasclUTgZYJY9aFNfAThX5CZRmczAV52oAPoupHhWRIUUAOoyUIlYVaAa/VbLbyiZUiyFbjQFNwiZQSGl4IDy9sO5Wrty0QLKhdZPxmgGcDo8ejn+c/6eiK9poz15Kw7Dr/vN/z6W7q++091/AQYA5mZ8GYJ9K0AAAAAASUVORK5CYII= ") left top no-repeat;           }

a.up {   background : url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAmlJREFUeNpsU0toU0EUPfPysx/tTxuDH9SCWhUDooIbd7oRUUTMouqi2iIoCO6lceHWhegy4EJFinWjrlQUpVm0IIoFpVDEIthm0dpikpf3ZuZ6Z94nrXhhMjM3c8895977BBHB2PznK8WPtDgyWH5q77cPH8PpdXuhpQT4ifR9u5sfJb1bmw6VivahATDrxcRZ2njfoaMv+2j7mLDn93MPiNRMvGbL18L9IpF8h9/TN+EYkMffSiOXJ5+hkD+PdqcLpICWHOHc2CC+LEyA/K+cKQMnlQHJX8wqYG3MAJy88Wa4OLDvEqAEOpJd0LxHIMdHBziowSwVlF8D6QaicK01krw/JynwcKoEwZczewroTvZirlKJs5CqQ5CG8pb57FnJUA0LYCXMX5fibd+p8LWDDemcPZbzQyjvH+Ki1TlIciElA7ghwLKV4kRZstt2sANWRjYTAGzuP2hXZFpJ/GsxgGJ0ox1aoFWsDXyyxqCs26+ydmagFN/rRjymJ1898bzGzmQE0HCZpmk5A0RFIv8Pn0WYPsiu6t/Rsj6PauVTwffTSzGAGZhUG2F06hEc9ibS7OPMNp6ErYFlKavo7MkhmTqCxZ/jwzGA9Hx82H2BZSw1NTN9Gx8ycHkajU/7M+jInsDC7DiaEmo1bNl1AMr9ASFgqVu9MCTIzoGUimXVAnnaN0PdBBDCCYbEtMk6wkpQwIG0sn0PQIUF4GsTwLSIFKNqF6DVrQq+IWVrQDxAYQC/1SsYOI4pOxKZrfifiUSbDUisif7XlpGIPufXd/uvdvZm760M0no1FZcnrzUdjw7au3vu/BVgAFLXeuTxhTXVAAAAAElFTkSuQmCC ") left top no-repeat;      }
"""

def dead_text(count):
    if count == 0:
        return ""
    return str(count)

def dead_file(filename, version):
    html = open(filename + ".html", "w+")
    deads = glob.glob(filename+"*globally*dead") + glob.glob(filename + "*globally*undead")

    p = subprocess.Popen("code2html -n " + filename, shell = True,
                         stdin=subprocess.PIPE, stdout=html, stderr=subprocess.PIPE)

    p.stdin.write('<script type="text/javascript" src="/Research/VAMOS/linux-trees/%s/jquery.js"></script>\n' % version);
    p.stdin.write('<script type="text/javascript" src="/Research/VAMOS/linux-trees/%s/explorer.js"></script>\n' % version);

    p.stdin.close()
    p.wait()
    return len(deads)

def dead_directory(dir=".", version = ""):
    fd = open(os.path.join(dir, "index.html"), "w+")
    deads = 0


    fd.write("""<html>
<head>
<style type="text/css">
""" + icon_file_css + """
</style>
</head>
<body>""")
    fd.write("<h1>Index for /%s</h1><hr>" % dir.replace("./", ""))
    fd.write("<table>")
    fd.write("<tr><td></td><td><a href='../index.html' class='icon up'>[parent directory]</a></td></tr>\n")

    paths = [path for path in os.listdir(dir) if os.path.isdir(os.path.join(dir, path))]
    paths_ = [os.path.join(dir, path) for path in paths]
    counts = map(lambda x:dead_directory(x, version), paths_)
    deads += sum(counts)
    for (path, d) in zip(paths, counts):
            fd.write("<tr><td>%s</td><td><a href='%s/index.html' class='icon dir'>%s</a></td></tr>\n" %(dead_text(d), path, path))


    files = glob.glob(dir + "/*.h") + glob.glob(dir + "/*.c")
    counts = map(lambda x: dead_file(x, version), files)
    deads += sum(counts)
    for (filename, d) in zip(files, counts):
        f = os.path.basename(filename)
        fd.write("<tr><td>%s</td><td><a href='%s.html' class='icon file'>%s</a></td></tr>\n" %(dead_text(d), f, f))

    fd.write("</ul>")

    fd.close()
    return deads

if __name__ == '__main__':
    print dead_directory(sys.argv[1], sys.argv[2])
