#!/bin/sh

em="tfogal@sci.utah.edu"
full_em="tfogal@sci.utah.edu jens@sci.utah.edu"

function try
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." | mail -s "Argon nightly FAILED" ${em}
        exit 1
    fi
}

status="status-argon"

echo "Using compiler version:" > ${status}
g++ --version >> ${status}
echo "" >> ${status}

echo "On system:" >> ${status}
uname -a >> ${status}
echo "" >> ${status}

echo "-------------------------------------" >> ${status}

try sh nightly.sh
cat warnings >> ${status}
subj="Argon nightly warnings -- `date`"
cat ${status} | mail -s "${subj}" ${full_em}
try scp *.tar.gz tfogal@shell.sci.utah.edu:/usr/sci/projects/sciweb/devbuilds/imagevis3d/
