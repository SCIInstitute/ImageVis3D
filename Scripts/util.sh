#!/bin/sh
# Various utility functions useful in nightly scripts.

# Give an error message and exit uncleanly.
function die
{
    echo "'$@' failed, bailing..."
    exit 1
}

# Run a command which must succeed, else the script terminates.
function try
{
    $@
    if test $? -ne 0 ; then
        die "$@"
    fi
}


# Determines the appropriate VCS system to use.  Mainly to handles/distinguish
# between svn and git-svn repositories.  Sets variable `VCS' to the appropriate
# executable.
VCS=""
function _vcs
{
    if test -d .git ; then
        VCS="git svn"
    else
       VCS="svn"
    fi
}

# Does the appropriate update, based on $VCS.  For git, assumes you do your
# personal work in a branch `private'
function vcs_update
{
    if test -z "${VCS}" ; then
        _vcs
    fi
    if test "x${VCS}" = "xsvn" ; then
        svn update
    else
        git stash save "uncomitted changes from `date`"
        git checkout master
        $VCS rebase
        git checkout private
        git rebase master

        pushd Tuvok
            git stash save "uncomitted changes from `date`"
            $VCS rebase
            git checkout private
            git rebase master
        popd
    fi
}

# Echoes the revision numbers from the two repositories.  Also sets
# R_IMAGEVIS3D and R_TUVOK shell variables.
function revision
{
    if test -z "${VCS}" ; then
        _vcs
    fi
    R_IMAGEVIS3D=`$VCS info | grep Revision | awk '{print $2}'`
    pushd Tuvok &>/dev/null
        R_TUVOK=`$VCS info | grep Revision | awk '{print $2}'`
    popd &> /dev/null
    echo "${R_IMAGEVIS3D}_${R_TUVOK}"
}

# Determine the architecture name according SCI conventions.  Basically, this
# is one of (Linux|osx|Win) with one of (32|64) appended.
function sci_arch
{
    local arch=`uname -m`
    if test "x${arch}" = "xi386" ; then
        arch="32"
    elif test "x${arch}" = "xx86_64" ; then
        arch="64"
    fi
    # else who knows .. just stick with the uname output, there's no convention
    # anymore anyway.
    echo "${arch}"
}
