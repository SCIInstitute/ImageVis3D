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

echo -en "Removing subversion garbage ...\t"
find "${PREFIX}" -iname .svn -exec rm -fr {} +
echo "done!"

macdeployqt="macdeployqt"
if test -n "${QT_BIN}" -a -x "${QT_BIN}/macdeployqt" ; then
    macdeployqt="${QT_BIN}/macdeployqt"
fi
echo "Running Qt's mac deployment tool."
${macdeployqt} ${PREFIX}

echo "Copying ImageVis3D Manual into app.."
pushd ${PREFIX}/Contents/Resources
  rm -f ImageVis3D.pdf
  gforge="https://gforge.sci.utah.edu/gf"
  manual="${gforge}/download/docmanfileversion/7/124/ImageVis3D.pdf"
  curl -kLO ${manual}
popd

echo "Fixing the errors that Qt's mac deployment tool doesn't."
for pgn in libqgif.dylib libqjpeg.dylib libqtiff.dylib ; do
    install_name_tool -change \
        @executable_path/../Frameworks/${pgn} \
        @executable_path/../PlugIns/imageformats/libqgiff.dylib \
        ${PREFIX}/Contents/MacOS/ImageVis3D || \
        echo "install_name_tool failed for ${pgn}; probably fine."
done
