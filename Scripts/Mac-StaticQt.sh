#!/bin/sh

rm -fr qt-mac-opensource-src-4.4.3
tar jxvf qt-mac-opensource-src-4.4.3.tar.bz2
pushd qt-mac-opensource-src-4.4.3
echo "yes" | \
./configure \
        -prefix ${HOME}/sw \
        -universal \
        -static \
        -qt-libjpeg \
        -no-openssl \
        -no-qt3support \
        -no-phonon \
        -no-webkit \
        -release \
        -no-sql-sqlite \
        -qt-gif
        -no-gif \
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
