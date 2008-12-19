#!/bin/bash

TARGETPATH=Build
TARGETAPP=ImageVis3D.app
PREFIX="${TARGETPATH}/${TARGETAPP}"

if test -n "$1" -a -d "$1" ; then
    echo "Using '$1' as .app directory."
    PREFIX="$1"
fi

function error
{
    echo "$@"
    exit 1
}
function mk_dir
{
    if test -d "$@" ; then
        echo "Using existing directory: $@"
    else
        echo "Creating '$@' directory."
        mkdir -p "$@"
    fi
}

if ! test -d "${PREFIX}" ; then
    error "$PREFIX does not exist, build the application first!"
fi

echo "Copying Shaders ..."
rm -fr "${PREFIX}/Contents/Resources"
mkdir -p "${PREFIX}/Contents/Resources"
cp tuvok/Shaders/* "${PREFIX}/Contents/Resources"

echo -en "Removing subversion garbage ...\t"
find "${PREFIX}" -iname .svn -exec rm -fr {} +

echo "done!"
