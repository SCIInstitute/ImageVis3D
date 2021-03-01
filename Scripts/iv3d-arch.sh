#!/bin/bash
# This exists so that CI can easily grab the arch string we'll embed into
# tarballs and the like.
source Scripts/util.sh 2>/dev/null|| source util.sh || exit 1

sci_arch
