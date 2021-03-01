#!/bin/sh
source Scripts/util.sh

spec="linux-g++"
if test `uname` = "Darwin" ; then
    spec="macx-g++"
fi
vcs_update

version
revision

if test "x$1" != "x--dirty" ; then
    make clean &>/dev/null
    # manual clean, just in case Qt's clean isn't good enough (it isn't.)
    find . \( -iname \*.o -or -iname moc_\*.cpp -or -iname ui_\*.h \) -delete
fi
rm -fr Build/ImageVis3D.app
rm -f Build/ImageVis3D warnings

# Find qmake -- expect it in PATH, but the user can set QT_BIN to pick a
# specific Qt.
if test -n "${QT_BIN}" -a -x "${QT_BIN}/qmake" ; then
    qmake="${QT_BIN}/qmake"
    echo "QT_BIN set; using ${qmake} instead of `which qmake`"
else
    qmake="qmake"
fi
# use qmake to generate makefiles, potentially in debug mode.
D_TUVOK="-DTUVOK_SVN_VERSION=${R_TUVOK}"
D_IV3D="-DIV3D_SVN_VERSION=${R_IMAGEVIS3D}"
CF="-fno-strict-aliasing ${D_TUVOK} ${D_IV3D} -U_DEBUG -DNDEBUG"
CFG="release"
if test "x$1" = "x-debug"; then
    CF="${CF} -Wextra -D_GLIBCXX_DEBUG -D_DEBUG -UNDEBUG -g"
    CFG="debug"
fi
${qmake} \
    QMAKE_CONFIG=${CFG} \
    QMAKE_CFLAGS="${CF}" \
    QMAKE_CXXFLAGS+="${CF}" \
    QMAKE_LFLAGS="${CF} ${LDF} ${LDFLAGS}" \
    -spec ${spec} \
    -recursive
if test $? -ne 0 ; then
    die "qmake failed."
fi
make -j3 2> warnings
try make

tarball=$(nm_tarball)
zipfile=$(nm_zipfile)
if test `uname` = "Darwin" ; then
    echo "Building app file ..."
    try bash Scripts/mk_app.sh
elif test `uname` = "Linux" ; then
    echo "Packaging ..."
    try bash Scripts/mk_tarball.sh
fi
rm -f latest
echo "${IV3D_MAJOR}.${IV3D_MINOR}.${IV3D_PATCH}" > latest
echo "${R_IMAGEVIS3D}" >> latest
echo "${TUVOK_MAJOR}.${TUVOK_MINOR}.${TUVOK_PATCH}" >> latest
echo "${R_TUVOK}" >> latest

if test -f warnings ; then
    echo "Warnings:"
    cat warnings
fi
