#!/bin/sh
source Scripts/util.sh

spec="linux-g++"
if test `uname` = "Darwin" ; then
    spec="macx-g++"
fi
vcs_update

# now that we've set eol:native in svn, the awk part might not be necessary.
IV3D_VERSION=` \
    grep "IV3D_VERSION " ImageVis3D/StdDefines.h | \
    awk '{ sub("\r", "", $3); print $3 }'`

make clean &>/dev/null
# manual clean, just in case Qt's clean isn't good enough.
find . \( -iname \*.o -or -iname moc_\*.cpp -or -iname ui_\*.h \) \
    -exec rm {} +
rm -fr Build/ImageVis3D.app
rm -f Build/ImageVis3D

# Find qmake -- expect it in PATH, but the user can set QT_BIN to pick a
# specific Qt.
if test -n "${QT_BIN}" -a -x "${QT_BIN}/qmake" ; then
    qmake="${QT_BIN}/qmake"
    echo "QT_BIN set; using ${qmake} instead of `which qmake`"
else
    qmake="qmake"
fi
# use qmake to generate makefiles, potentially in debug mode.
CF="-fno-strict-aliasing"
CFG="release"
if test "x$1" = "x-debug"; then
    CF="${CF} -Wextra -D_GLIBCXX_DEBUG"
    CFG="debug"
fi
${qmake} \
    CONFIG+=${CFG} \
    QMAKE_CFLAGS="${CF}" \
    QMAKE_CXXFLAGS+="${CF}" \
    QMAKE_LDFLAGS="${CF} ${LDF}" \
    -spec ${spec} \
    -recursive
if test $? -ne 0 ; then
    die "qmake failed."
fi
make clean
rm -f warnings
make -j5 2> warnings
try make

# Get the current revisions of the two repositories, so we can appropriately
# label the build tarballs.
revs=$(revision)

# likewise for the machine architecture.
arch=$(sci_arch)

tarball=""
if test `uname` = "Darwin" ; then
    echo "Building app file ..."
    tarball="ImageVis3D_${IV3D_VERSION}_osx${arch}_r${revs}.tar.gz"
    try bash Scripts/mk_app.sh
    pushd Build/ &>/dev/null
        tar zcf ${tarball} ImageVis3D.app
        zip -r ${tarball%%.tar.gz}.zip ImageVis3D.app
    popd &>/dev/null
    mv Build/${tarball} Build/${tarball%%.tar.gz}.zip .
elif test `uname` = "Linux" ; then
    mkdir staging
    pushd staging
        dir="ImageVis3D_${IV3D_VERSION}"
        mkdir ${dir}
        cp ../Build/ImageVis3D ./${dir}
        cp -R ../Tuvok/Shaders ./${dir}
        tar zcf "${dir}_Linux_r${revs}.tar.gz" ${dir}
        mv "${dir}_Linux_r${revs}.tar.gz" ../
    popd
    rm -r staging
fi

echo "Warnings:"
cat warnings
