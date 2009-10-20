#!/bin/sh

VERSION=4.5.3
echo "Removing old build..."
rm -fr qt-all-opensource-src-${VERSION}

tarball="qt-all-opensource-src-${VERSION}.tar"
echo "Extracting..."
# Do they have a bzip'd or a gzip'd tarball?
if test -f ${tarball}.bz2 ; then
    tar jxf ${tarball}.bz2
else
    tar zxf ${tarball}.gz
fi
pushd qt-all-opensource-src-${VERSION} || exit 1
echo "yes" | \
./configure \
        -prefix ${HOME}/sw \
        -opensource \
        -static \
        -qt-libjpeg \
        -no-openssl \
        -no-qt3support \
        -no-phonon \
        -no-webkit \
        -release \
        -no-sql-sqlite \
        -qt-gif \
        -no-framework \
        -make libs \
        -make tools

if test $? -ne 0; then
        echo "configure failed"
        exit 1
fi

nice make -j4 sub-src || exit 1
nice make install || exit 1

popd
