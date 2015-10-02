#! /bin/sh

DATE=`git log -1 --pretty="%ci"`
HASH=`git log -1 --pretty="%h"`
REF=`git log -1 --pretty="%D" | sed "s/\//\\\//g"`

VER="\"($HASH) $DATE ($REF)\""
echo $VER
echo OK?
read

sed "s#^\(PROJECT_NUMBER\s*=\).*#\1 ${VER}#g" Doxyfile.in > Doxyfile
doxygen
