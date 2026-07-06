#!/bin/sh
wget -r -l1 -H -nd -N --timeout=10 --tries=1 --reject "index.html*,*.cgi,*.shtml,*.html" -e robots=off http://www.j-a-r-n-o.nl/xpilotmaps.shtml
