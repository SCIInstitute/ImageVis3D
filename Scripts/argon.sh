#!/bin/sh

export PATH="/Users/tfogal/sw/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"

em="tfogal@sci.utah.edu"
full_em="tfogal@sci.utah.edu jens@sci.utah.edu"

status="status-argon"
function try
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." >> ${status}
        cat ${status} | mail -s "Argon nightly FAILED" ${em}
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

try cd ${HOME}/imagevis3d
rm *.tar.gz
try sh nightly.sh
cat warnings >> ${status}
subj=""
if test `file warnings | awk '{print $2}'` = "empty" ; then
    subj="Argon nightly (clean) -- `date`"
else
    subj="Argon nightly (warnings) -- `date`"
fi
cat ${status} | mail -s "${subj}" ${full_em}
try scp *.tar.gz *.zip \
    tfogal@shell.sci.utah.edu:/usr/sci/projects/sciweb/devbuilds/imagevis3d/
