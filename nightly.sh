#!/bin/sh

function try
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        exit 1
    fi
}
if test -d .git ; then
    if test -x `which tjf-git-svn`; then
        svn="tjf-git-svn"
    else
        svn="git svn"
    fi
else
    svn="svn"
fi

function update
{
    if test -d .git ; then
        git diff --quiet
        if test $? -ne 0 ; then
            git stash save "hacks"
            saved=1
        fi
        git checkout master
        tjf-git-svn rebase
        git checkout private
        git rebase master
        if test ${saved} -eq 1 ; then
            git stash pop
        fi
    else
        svn update
    fi
}

spec="linux-g++"
if test `uname` = "Darwin" ; then
    spec="macx-g++"
fi
update

# manual clean.
find . \( -iname \*.o -or -iname moc_\*.cpp -or -iname ui_\*.h \) \
    -exec rm {} +

try qmake -spec ${spec}
make clean
make -j5
try make

revision=`$svn info | grep Revision | awk '{print $2}'`
echo "revision: $revision"
if test `uname` = "Darwin" ; then
    echo "Building app file ..."
    try bash mk_app.sh
    pushd Build/OSX/Bin &>/dev/null
    tar zcf iv3d-osx-r${revision}.tar.gz ImageVis3D.app
    popd &>/dev/null
    mv Build/OSX/Bin/iv3d-osx-r${revision}.tar.gz .
elif test `uname` = "Linux" ; then
    mv Build/Linux/Bin/ImageVis3D ./ImageVis3D-r${revision}
fi
