#!/bin/bash
source Scripts/util.sh || source util.sh

TARGETPATH=Build
TARGETAPP=ImageVis3D.app
PREFIX="${TARGETPATH}/${TARGETAPP}"

if test -n "$1" -a -d "$1" ; then
    echo "Using '$1' as .app directory."
    PREFIX="$1"
fi

if ! test -d "${PREFIX}" ; then
    die "$PREFIX does not exist, build the application first!"
fi

echo "Copying Shaders ..."
rm -f "${PREFIX}/Contents/Resources/*.glsl"
mkdir -p "${PREFIX}/Contents/Resources"
cp tuvok/Shaders/* "${PREFIX}/Contents/Resources"

echo -en "Removing subversion garbage ...\t"
find "${PREFIX}" -iname .svn -exec rm -fr {} +

echo "done!"
