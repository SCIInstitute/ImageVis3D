#!/bin/sh
# Creates a tarball binary of ImageVis3D.
source Scripts/util.sh

tarball=$(nm_tarball)
mkdir staging
man=$(manual)
pushd staging >/dev/null
  ver="${IV3D_MAJOR}.${IV3D_MINOR}.${IV3D_PATCH}"
  dir="ImageVis3D-${ver}"
  mkdir "${dir}"
  cp ../Build/ImageVis3D ./${dir}
  cp -R ../Tuvok/Shaders ./${dir}
  wget -q --no-check-certificate "${man}"
  mv $(basename "${man}") ImageVis3D.pdf # uppercase it.
  mv ImageVis3D.pdf ${dir}
  GZIP="--best" tar zcf "${tarball}" ${dir}
  mv "${tarball}" ../
popd >/dev/null
rm -r staging
