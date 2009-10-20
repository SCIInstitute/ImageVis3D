#!/bin/bash
source Scripts/util.sh || source util.sh

TARGETPATH=Build
TARGETAPP=ImageVis3D.app
PREFIX="${TARGETPATH}/${TARGETAPP}"

if test -n "$1" -a -d "$1" ; then
    echo "Using '$1' as .app directory."
    PREFIX="$1"
fi

if ! test -d "${PREFIX}" ; then
    die "$PREFIX does not exist, build the application first!"
fi

echo "Copying Shaders ..."
rm -f "${PREFIX}/Contents/Resources/*.glsl"
mkdir -p "${PREFIX}/Contents/Resources"
cp tuvok/Shaders/* "${PREFIX}/Contents/Resources"

echo "Removing subversion garbage ..."
find "${PREFIX}" -iname .svn -exec rm -fr {} +
echo "done!"

macdeployqt="macdeployqt"
if test -n "${QT_BIN}" -a -x "${QT_BIN}/macdeployqt" ; then
    macdeployqt="${QT_BIN}/macdeployqt"
fi
echo "Running Qt's mac deployment tool."
${macdeployqt} ${PREFIX}

echo "Copying ImageVis3D Manual into app..."
man=$(manual)
pushd ${PREFIX}/Contents/Resources
  rm -f ImageVis3D.pdf
  curl -kLO "${man}"
  mv $(basename "${man}") ImageVis3D.pdf
popd

echo "Fixing the errors that Qt's mac deployment tool doesn't."
for pgn in libqgif.dylib libqjpeg.dylib libqtiff.dylib ; do
    install_name_tool -change \
        @executable_path/../Frameworks/${pgn} \
        @executable_path/../PlugIns/imageformats/${pgn} \
        ${PREFIX}/Contents/MacOS/ImageVis3D || \
        echo "install_name_tool failed for ${pgn}; probably fine."
done
