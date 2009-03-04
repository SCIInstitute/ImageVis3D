#!/bin/sh

PREFIX="${HOME}/sw"

# The important settings are a static build and ensuring we link in all of the
# sub-libraries (zlib, png, jpeg, etc.) internally, instead of relying on
# system versions.
# We also disable some features to make the build a bit quicker.
./configure            \
    -prefix ${PREFIX}  \
    -release           \
    -static            \
    -stl               \
    -no-sql-sqlite     \
    -no-sql-sqlite2    \
    -no-qt3support     \
    -no-phonon         \
    -no-webkit         \
    -qt-zlib           \
    -qt-gif            \
    -qt-libtiff        \
    -qt-libpng         \
    -qt-libmng         \
    -qt-libjpeg        \
    -openssl-linked    \
    -no-dbus           \
    -opengl            \
    -no-tablet

if test $? -ne 0 ; then
    echo "configure error"
    exit 1
fi

nice -n 19 make -j6 || exit 1

make install || exit 1
