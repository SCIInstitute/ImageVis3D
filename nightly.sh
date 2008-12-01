#!/bin/sh

function try
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        exit 1
    fi
}

spec="linux-g++"
if test `uname` = "Darwin" ; then
    spec="macx-g++"
fi
svn update

# manual clean.
find . \( -iname \*.o -or -iname moc_\*.cpp -or -iname ui_\*.h \) \
	-exec rm {} +

try qmake -spec ${spec}
make clean
make -j5
try make

if test `uname` = "Darwin" ; then
	echo "Building app file ..."
    try bash mk_app.sh
	revision=`svn info | grep Revision | awk '{print $2}'`
	pushd Build/OSX/Bin &>/dev/null
	tar zcf iv3d-osx-r${revision}.tar.gz ImageVis3D.app
	popd &>/dev/null
	mv Build/OSX/Bin/iv3d-osx-r${revision}.tar.gz .
fi
