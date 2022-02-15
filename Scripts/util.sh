#!/bin/sh
# Various utility functions useful in nightly scripts.

# this affects what BSD mail uses for the Reply-To header:
export REPLYTO="tfogal@sci.utah.edu"
# if something should fail:
em="tfogal@sci.utah.edu"
# warnings and other information:
full_em="tfogal@sci.utah.edu"

export status="status-${hostname}"

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

# like 'try' but sends an email if it fails.
function mailtry
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." >> ${status}
        if test -f warnings ; then
            echo "-------------------------------------" >> ${status}
            echo "" >> ${status}
            cat warnings >> ${status}
        fi
        cat ${status} | mail -s "$(hostname) nightly FAILED" ${em}
        exit 1
    fi
}

# Determines the appropriate VCS system to use.  Mainly to handles/distinguish
# between svn and git-svn repositories.  Sets variable `VCS' to the appropriate
# executable.
# We switched to github and thus always git in 2014.
VCS=""
function _vcs
{
    VCS="git"
}

# Updates a git-svn repository.  One argument: a directory to cd to first.
function git_rebase
{
    pushd "$@"
    git diff --exit-code --quiet &>/dev/null
    local saved=0
    if test $? -ne 0 ; then
        git stash save "uncomitted changes from `date`"
        saved=1
    fi
    git checkout master
    git pull --rebase
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
        git_rebase "."
    fi
}

# Echoes the revision numbers from the two repositories.  Also sets
# R_IMAGEVIS3D shell variable.
function revision
{
    if test -z "${VCS}" ; then
        _vcs
    fi
    R_IMAGEVIS3D=`git log --stat --pretty=one HEAD^..HEAD | head -n 1 | awk '{print $1}'`
    echo "${R_IMAGEVIS3D}"
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

# Determine the platform of the current machine.
function sci_arch
{
    local arch=`uname -m`
    local opsys=`uname -s`
    if test "x${opsys}" = "xDarwin" ; then
        # 10.4?  10.5?  This gets the system version.
        local sysver=$(system_profiler                          \
                         -detailLevel mini SPSoftwareDataType | \
                       grep "System Version:" |                 \
                       awk '{print $6}')
        opsys="${opsys}-${sysver}"
    fi
    if test `uname` = "Linux" ; then
      distro=$(lsb_release -i | awk '{print $3}')
      distro_release=$(lsb_release -r | awk '{print $2}')
      echo "${distro}-${distro_release}-${arch}"
    else
      echo "${opsys}-${arch}"
    fi
}

# Gives the name of the appropriate tarball.
function nm_tarball
{
    local arch=$(sci_arch)
    local revs=$(revision)
    if test ! -z "$IV3D_BUILD_TYPE" ; then
      local btype=$IV3D_BUILD_TYPE
    else
      local btype="na"
    fi
    version
    echo "ImageVis3D-${arch}-${IV3D_VERSION}-r${revs}-$btype.tar.gz"
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

# Returns the current location of the data manual
function import_data_manual
{
  echo "http://hpc.uni-due.de/data/IV3D/GettingDataIntoImageVis3D.pdf"
}
