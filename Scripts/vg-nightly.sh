#!/bin/sh
# Run valgrind on the converter.  Intended for automatic operation.
. Scripts/util.sh

if ! test -f aneur.nrrd ; then
    wget http://www.sci.utah.edu/~tfogal/aneur.nrrd
fi

revs=$(revision)
echo "revision: ${revs}"
valgrind --leak-check=no --undef-value-errors=yes \
    --log-file="vg-log-${revs}" \
     CmdLineConverter/Build/UVFConverter \
        -f aneur.nrrd \
        -out tmp.uvf
if test $? -ne 0 ; then
    echo "Abnormal termination."
fi

rm -f tmp.uvf
