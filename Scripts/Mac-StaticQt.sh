#!/bin/sh

tarball="qt-mac-opensource-src-4.4.3.tar"
# If the user re-compressed it as bzip2, use that.
if test -f ${tarball}.bz2 ; then
    tarball="${tarball}.bz2"
else
    tarball="${tarball}.gz"
fi
rm -fr qt-mac-opensource-src-4.4.3
tar jxf ${tarball}
pushd qt-mac-opensource-src-4.4.3
echo "yes" | \
./configure \
        -prefix ${HOME}/sw \
        -universal \
        -arch x86 \
        -arch ppc \
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

make sub-src || exit 1
make install || exit 1

popd
