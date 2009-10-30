#!/bin/sh
# A script to checkout and create .debs of the ImageVis3D repository.
. util.sh || . Scripts/util.sh

mode="debian"
if test "$1" = "--ubuntu-ppa" ; then
  mode="ubuntu-ppa"
fi

version
VER="${IV3D_MAJOR}.${IV3D_MINOR}.${IV3D_PATCH}"
DEB_VER=1
SVN=https://gforge.sci.utah.edu/svn/imagevis3d

function die {
  echo "$@"
  exit 1
}

rm -fr imagevis3d-${VER}
rm -f ./*.changes ./*.deb ./*.tar.gz ./*.dsc

revs=$(svn export --force ${SVN} | \
       grep "Exported" |           \
       awk '{print $(NF)'} |       \
       cut -d. -f1 |               \
       tr "[:space:]" "~"          \
      )
if test $? -ne 0; then die "svn export failed." ; fi
echo "revs: ${revs}"

mv imagevis3d imagevis3d-${VER}
tar zcf imagevis3d_${VER}.orig.tar.gz imagevis3d-${VER}

if test "${mode}" = "ubuntu-ppa" ; then
  DEB_VER="${DEB_VER}~ppa~svn${revs}"
else
  DEB_VER="${DEB_VER}~svn${revs}"
fi

pushd imagevis3d-${VER}
  #ln -s notdebian debian || exit 1
  cp -R ../../notdebian debian || exit 1
  # Generate a good changelog.
  pushd debian
    if test "${mode}" = "ubuntu-ppa" ; then
      distroseries="karmic"
    else
      distroseries="unstable"
    fi
    echo "imagevis3d (${VER}-${DEB_VER}) ${distroseries}; urgency=low" > ncl
    echo "" >> ncl
    echo "  * Nightly build." >> ncl
    echo "" >> ncl
    echo " -- Thomas Fogal <tfogal@alumni.unh.edu>  $(date -R)" >> ncl
    echo "" >> ncl
    cat changelog >> ncl
    mv ncl changelog
    # So one can review what that generated:
    echo "changelog:"
    head changelog
  popd
  if test "${mode}" = "ubuntu-ppa" ; then
    dpkg-buildpackage -rfakeroot -sgpg -S -sa || exit 1
  else
    dpkg-buildpackage -rfakeroot -sgpg -sa -j4 || exit 1
  fi
popd

if test "${mode}" = "ubuntu-ppa" ; then
  lintian -I -i imagevis3d_${VER}-${DEB_VER}_source.changes | less
  read -p "dput to launchpad? " upload
  if test "${upload}" = "y"; then
    dput -f sci-ppa imagevis3d_${VER}-${DEB_VER}_source.changes
  fi
else
  lintian -I -i imagevis3d_${VER}-${DEB_VER}_amd64.changes
fi
