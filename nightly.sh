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
        saved=0
        if test $? -ne 0 ; then
            git stash save "hacks"
            saved=1
        fi
        git checkout master
        $svn rebase
        git checkout private
        git rebase master

        pushd Tuvok
            git checkout -f master
            $svn rebase
            git checkout private
            git rebase master
        popd
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

if test "x$1" = "-debug"; then
    try qmake -spec ${spec} -recursive
    CF="-Wextra -D_GLIBCXX_DEBUG"
    try qmake CONFIG+=debug QMAKE_CXXFLAGS+="${CF}" -recursive
else
    try qmake -spec ${spec} -recursive
fi
make clean
make -j5 2> warnings
try make

revision=`$svn info | grep Revision | awk '{print $2}'`
echo "revision: $revision"
tarball=""
if test `uname` = "Darwin" ; then
    echo "Building app file ..."
    tarball="ImageVis3D_0.02b_OSX_r${revision}.tar.gz"
    try bash mk_app.sh
    pushd Build/ &>/dev/null
        tar zcf ${tarball} ImageVis3D.app
    popd &>/dev/null
    mv Build/${tarball} .
elif test `uname` = "Linux" ; then
    mv Build/ImageVis3D ./ImageVis3D-r${revision}
fi

echo "Warnings:"
cat warnings
