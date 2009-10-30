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

# Updates a git-svn repository.  One argument: a directory to cd to first.
function git_svn_update
{
    pushd "$@"
    git diff --exit-code --quiet &>/dev/null
    local saved=0
    if test $? -ne 0 ; then
        git stash save "uncomitted changes from `date`"
        saved=1
    fi
    git checkout master
    $VCS rebase
    git checkout private
    git rebase master
    if test $saved -eq 1 ; then
        git stash pop
    fi

    popd
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
        git_svn_update "."
        git_svn_update "Tuvok"
        git_svn_update "Tuvok/Basics"
        git_svn_update "Tuvok/IO"
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

# Reads the version numbers from Std*Defines.h.  Sets IV3D_VERSION,
# TUVOK_VERSION.
function version
{
    # search for StdDefs: we might be in the Scripts/ dir.
    if test -f "ImageVis3D/StdDefines.h" ; then
      vheader="ImageVis3D/StdDefines.h"
    else
      vheader="../ImageVis3D/StdDefines.h"
    fi

    export IV3D_MAJOR=` \
        grep "IV3D_MAJOR" ${vheader} | \
        awk '{ print $3 }'`
    export IV3D_MINOR=` \
        grep "IV3D_MINOR" ${vheader} | \
        awk '{ print $3 }'`
    export IV3D_PATCH=` \
        grep "IV3D_PATCH" ${vheader} | \
        awk '{ print $3 }'`

    if test -f "Tuvok/StdTuvokDefines.h" ; then
      vheader="Tuvok/StdTuvokDefines.h"
    else
      vheader="../Tuvok/StdTuvokDefines.h"
    fi

    export TUVOK_MAJOR=` \
        grep "TUVOK_MAJOR" ${vheader} | \
        awk '{ print $3 }'`
    export TUVOK_MINOR=` \
        grep "TUVOK_MINOR" ${vheader} | \
        awk '{ print $3 }'`
    export TUVOK_PATCH=` \
        grep "TUVOK_PATCH" ${vheader} | \
        awk '{ print $3 }'`

    export IV3D_VERSION="${IV3D_MAJOR}.${IV3D_MINOR}.${IV3D_PATCH}"
    export TUVOK_VERSION="${TUVOK_MAJOR}.${TUVOK_MINOR}.${TUVOK_PATCH}"
}

# Determine the architecture name according SCI conventions.  Basically, this
# is one of (Linux|osx|Win) with one of (32|64) appended.
function sci_arch
{
    local arch=`uname -m`
    local opsys=`uname -s`
    if test "x${opsys}" = "xDarwin" ; then
        opsys="osx"
        # 10.4?  10.5?  This gets the system version.
        local sysver=$(system_profiler                          \
                         -detailLevel mini SPSoftwareDataType | \
                       grep "System Version:" |                 \
                       awk '{print $6}')
        opsys="${opsys}${sysver}"
    fi
    if test "x${arch}" = "xi386" -o "x${arch}" = "xi686" ; then
        arch="32bit"
    elif test "x${arch}" = "xx86_64" ; then
        arch="64bit"
    fi
    # else who knows .. just stick with the uname output, there's no convention
    # anymore anyway.
    echo "${opsys}-${arch}"
}

# Gives the name of the appropriate tarball.
function nm_tarball
{
    local arch=$(sci_arch)
    local revs=$(revision)
    version
    echo "ImageVis3D_${IV3D_VERSION}_${arch}_r${revs}.tar.gz"
}

# Gives the name of the appropriate zip file.
function nm_zipfile
{
    local tb_name=$(nm_tarball)
    echo "${tb_name%%.tar.gz}.zip"
}

# Finds out the current URL for the manual.
function manual
{
  m=$(grep HELP_URL ./ImageVis3D/StdDefines.h | awk '{print $3}' | \
      cut -d \" -f 2)
  echo "${m}"
}
