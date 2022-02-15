#!/bin/bash
MAJOR=4
MINOR=8
PATCH=6
VERSION=${MAJOR}.${MINOR}.${PATCH}
SOURCEDIR="${HOME}/Qt/Source"
BUILDDIR="${HOME}/Qt/Build"
PREFIX=${BUILDDIR}/${VERSION}
QTDIR="qt-everywhere-opensource-src-${VERSION}"

mkdir -p ${SOURCEDIR}
mkdir -p ${BUILDDIR}

pushd ${SOURCEDIR} || exit 1

echo "Removing old sources and build..."
rm -fr ${QTDIR}

TARBALL="${QTDIR}.tar"

# Do they have a bzip'd or a gzip'd tarball?
if test -f ${TARBALL}.bz2 ; then
  echo "Extracting..."
  tar jxf ${TARBALL}.bz2
elif test -f ${TARBALL}.gz ; then
  echo "Extracting..."
  tar zxf ${TARBALL}.gz
else
  echo "${TARBALL}.gz not found; Downloading Qt..."
  curl -kLO http://download.qt-project.org/archive/qt/${MAJOR}.${MINOR}/${VERSION}/${TARBALL}.gz
  echo "Extracting..."
  tar zxf ${TARBALL}.gz
fi

pushd ${QTDIR} || exit 1
echo "yes" | \
./configure \
        -prefix ${PREFIX} \
        -buildkey "imagevis3d" \
        -release \
        -opensource \
        -largefile \
        -exceptions \
        -fast \
        -stl \
        -no-qt3support \
        -no-xmlpatterns \
        -no-multimedia \
        -no-audio-backend \
        -no-phonon \
        -no-phonon-backend \
        -no-svg \
        -no-webkit \
        -no-javascript-jit \
        -no-script \
        -no-scripttools \
        -no-declarative \
        -no-declarative-debug \
        -platform unsupported/macx-clang \
        -no-scripttools \
        -system-zlib \
        -no-gif \
        -qt-libtiff \
        -qt-libpng \
        -qt-libmng \
        -qt-libjpeg \
        -no-openssl \
        -make libs \
        -make tools \
        -nomake examples \
        -nomake demos \
        -nomake docs \
        -nomake translations \
        -no-nis \
        -no-cups \
        -no-iconv \
        -no-pch \
        -no-dbus \
        -arch "x86 x86_64"

if test $? -ne 0; then
        echo "configure failed"
        exit 1
fi

nice make -j6 || exit 1

echo "Removing old install..."
rm -fr ${PREFIX}

nice make install || exit 1

echo "Creating symlink to last install..."
ln -s ${PREFIX} ${BUILDDIR}/Current

popd
popd
