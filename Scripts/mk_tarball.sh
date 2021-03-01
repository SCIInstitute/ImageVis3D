#!/bin/bash
# Creates a tarball binary of ImageVis3D.
if test ! -z "$1" ; then
  IV3D_BUILD_TYPE=$1
fi
source Scripts/util.sh

tarball=$(nm_tarball)
mkdir staging
man=$(manual)
import=$(import_data_manual)
version
pushd staging >/dev/null
  ver="${IV3D_MAJOR}.${IV3D_MINOR}.${IV3D_PATCH}"
  dir="ImageVis3D-${ver}"
  mkdir "${dir}"
  cp ../Build/ImageVis3D ./${dir}
  cp ../CmdLineConverter/Build/uvfconvert ${dir}
  cp -R ../Tuvok/Shaders ./${dir}
  rm -fr ${dir}/Shaders/.svn # remove dumb svn crap

  # grab manuals
  wget -q --no-check-certificate "${man}"
  mv $(basename "${man}") ImageVis3D.pdf # uppercase it.
  mv ImageVis3D.pdf ${dir}
  wget -q --no-check-certificate "${import}"
  mv $(basename "${import}") ${dir}

  # tar it all up
  GZIP="--best" tar zcf "${tarball}" ${dir}
  mv "${tarball}" ../
popd >/dev/null
rm -fr staging
