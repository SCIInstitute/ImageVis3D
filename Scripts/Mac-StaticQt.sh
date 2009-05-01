#!/bin/sh

VERSION=4.5.1
echo "Removing old build..."
rm -fr qt-mac-opensource-src-${VERSION}

tarball="qt-mac-opensource-src-${VERSION}.tar"
echo "Extracting..."
# Do they have a bzip'd or a gzip'd tarball?
if test -f ${tarball}.bz2 ; then
    tar jxf ${tarball}.bz2
else
    tar zxf ${tarball}.gz
fi
pushd qt-mac-opensource-src-${VERSION} || exit 1
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
        -fast

if test $? -ne 0; then
        echo "configure failed"
        exit 1
fi

time nice make -j4 sub-src || exit 1
make install || exit 1

popd
