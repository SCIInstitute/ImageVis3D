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
REPO=https://github.com/SCIInstitute/ImageVis3D.git

function die {
  echo "$@"
  exit 1
}

rm -fr imagevis3d-${VER}*
rm -f ./*.changes ./*.deb ./*.tar.gz ./*.dsc ./*.diff.gz ./*_source.upload \
  ./*_source.changes

rm -fr ImageVis3D # kill previous clone
git clone --depth 1 ${REPO} > /dev/null || die "clone failed"
(cd ImageVis3D && git submodule init) || die "could not init submodules"
(cd ImageVis3D && git submodule update) || die "submodules couldn't be updated"

if test "${mode}" = "ubuntu-ppa" ; then
  VER="${VER}~ppa"
else
  VER="${VER}~git"
fi

rm -fr "imagevis3d-${VER}"
mv ImageVis3D "imagevis3d-${VER}" || die "could not move dir"
export GZIP="--best"
tar zcf imagevis3d_${VER}.orig.tar.gz imagevis3d-${VER}

pushd imagevis3d-${VER} || die "imagevis3d-${VER} directory does not exist"
  if test -d ../../notdebian ; then
    echo "Running from Scripts/?  Using local debian dir."
    cp -R ../../notdebian debian || die "couldn't copy debian dir"
  else
    echo "Not running from scripts; using in-repo debian dir."
    ln -s notdebian debian || die "couldn't symlink debian dir"
  fi

  # Generate a valid changelog.
  pushd debian || die "no debian dir."
    if test "${mode}" = "ubuntu-ppa" ; then
      distroseries="saucy"
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
    dpkg-buildpackage -rfakeroot -sgpg -S -sa || die "buildpackage failed."
  else
    dpkg-buildpackage -rfakeroot -sgpg -sa -j4 || die "buildpackage failed"
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
