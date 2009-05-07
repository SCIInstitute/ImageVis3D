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
    # manual clean, just in case Qt's clean isn't good enough.
    find . \( -iname \*.o -or -iname moc_\*.cpp -or -iname ui_\*.h \) \
        -exec rm {} +
    rm -fr Build/ImageVis3D.app
    rm -f Build/ImageVis3D
    rm -f warnings
fi

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
CF="-fno-strict-aliasing ${D_TUVOK} ${D_IV3D}"
CFG="release"
if test "x$1" = "x-debug"; then
    CF="${CF} -Wextra -D_GLIBCXX_DEBUG -g"
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
make -j5 2> warnings
try make

tarball=$(nm_tarball)
zipfile=$(nm_zipfile)
if test `uname` = "Darwin" ; then
    echo "Building app file ..."
    try bash Scripts/mk_app.sh
    pushd Build/ &>/dev/null
        sed -i -e "s,VERSION,${IV3D_VERSION}," \
          ImageVis3D.app/Contents/Info.plist
        tar zcf ${tarball} ImageVis3D.app
        zip -9r ${zipfile} ImageVis3D.app
    popd &>/dev/null
    mv Build/${tarball} Build/${zipfile} .
    hdiutil create                    \
      -volname "ImageVis3D"           \
      -srcfolder Build/ImageVis3D.app \
      -format UDZO                    \
      -imagekey zlib-level=9          \
      ${zipfile%%zip}dmg
    hdiutil internet-enable -yes ${zipfile%%zip}dmg
elif test `uname` = "Linux" ; then
    mkdir staging
    pushd staging
        dir="ImageVis3D_${IV3D_VERSION}"
        mkdir "${dir}"
        cp ../Build/ImageVis3D ./${dir}
        cp -R ../Tuvok/Shaders ./${dir}
        GZIP="--best" tar zcf "${tarball}" ${dir}
        mv "${tarball}" ../
    popd
    rm -r staging
fi
rm -f latest
echo "${IV3D_VERSION}" > latest
echo "${R_IMAGEVIS3D}" >> latest
echo "${TUVOK_VERSION}" >> latest
echo "${R_TUVOK}" >> latest

echo "Warnings:"
cat warnings
