#!/bin/bash

{
    echo "WikiStart";
    wget -q -O- http://vamos.informatik.uni-erlangen.de/trac/undertaker/wiki/TitleIndex | \
        sed 's/</\n/g' | egrep -o "Undertaker[^<>\" ][^<\">]*" | sort -u | grep -v Release; } | \
    while read page; do
    echo "Extracting $page"
    wget -q -O "$page" http://vamos.informatik.uni-erlangen.de/trac/undertaker/wiki/"$page"?format=txt
done

mv WikiStart README