#!/bin/sh

# See: http://longair.net/blog/2011/04/09/missing-git-hooks-documentation/

# Check to see if there are any modifications in the ./Tuvok directory.
unset GIT_DIR
unset GIT_INDEX_FILE
CHANGES=`git --git-dir=Tuvok/.git --work-tree=Tuvok status --porcelain`

if [ "$CHANGES" == "" ]; then
  exit 0
else
  echo "[POLICY] You cannot make changes to Tuvok in the ImageVis3D repo.\n"
  echo "Please clone Tuvok separately, make your changes, then update\nthe Tuvok submodule in ImageVis3D. To update the submodule,\nuse 'git checkout master' followed by 'git pull' in the Tuvok\nsubmodule directory."
  exit 1
fi

