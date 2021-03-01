#!/bin/sh

if test -z "${IV3D_DATA_ROOT}" ; then
  echo "IV3D_DATA_ROOT environment variable not set.  Bailing."
  exit 1
fi

unison \
  -batch \
  ${IV3D_DATA_ROOT} \
  socket://155.98.20.225:43212/iv3d-test-data || exit 1
