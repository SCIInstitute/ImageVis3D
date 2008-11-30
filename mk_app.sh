#!/bin/bash

TARGETPATH=Build/OSX/Bin
TARGETAPP=ImageVis3D.app
PREFIX="${TARGETPATH}/${TARGETAPP}"

function error
{
    echo "$@"
    exit 1
}
function mk_dir
{
    if test -d "$@" ; then
        echo "Using existing directory: $@"
    else
        echo "Creating '$@' directory."
        mkdir -p "$@"
    fi
}

if ! test -d "${PREFIX}" ; then
    error "$PREFIX does not exist, build the application first!"
fi

mk_dir "${PREFIX}/Contents/Frameworks"

for fw in QtCore.framework QtGui.framework QtOpenGL.framework ; do
    echo "Removing ${PREFIX}/Contents/Frameworks/${fw}"
    rm -fr "${PREFIX}/Contents/Frameworks/${fw}"
done

echo "Copying framework files ..."
echo -e "\t Core"
cp -R /Library/Frameworks/QtCore.framework "${PREFIX}/Contents/Frameworks"

echo -e "\t GUI"
cp -R /Library/Frameworks/QtGui.framework "${PREFIX}/Contents/Frameworks"

echo -e "\t QtOpenGL"
cp -R /Library/Frameworks/QtOpenGL.framework "${PREFIX}/Contents/Frameworks"

echo "Copying Shaders ..."
rm -fr "${PREFIX}/Contents/Resources"
mkdir -p "${PREFIX}/Contents/Resources"
cp Shaders/* "${PREFIX}/Contents/Resources"

echo "Running install_name_tool"

# FIXME These should probably be one for loop ..

# set the identification names for the frameworks
for fw in QtCore QtGui QtOpenGL ; do
    install_name_tool -id \
        @executable_path/../Frameworks/${fw}.framework/Versions/4/${fw} \
        ${PREFIX}/Contents/Frameworks/${fw}.framework/Versions/4/${fw}
done

# change "pointers" to the frameworks
for fw in QtCore QtGui QtOpenGL ; do
    install_name_tool -change \
        ${fw}.framework/Versions/4/QtCore \
        @executable_path../Frameworks/${fw}.framework/Versions/4/${fw} \
        ${PREFIX}/Contents/MacOs/ImageVis3D
done

# change "pointers" within the frameworks cross referencing to core
for fw in QtCore QtGui QtOpenGL ; do
    install_name_tool -change \
        ${fw}.framework/Versions/4/${fw} \
        @executable_path/../Frameworks/${fw}.framework/Versions/4/${fw} \
        ${PREFIX}/Contents/Frameworks/${fw}.framework/Versions/4/${fw}
done
