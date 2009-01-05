#!/bin/sh

export PATH="${HOME}/sw/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"
# this affects what BSD mail uses for the Reply-To header:
export REPLYTO="tfogal@sci.utah.edu"

em="tfogal@sci.utah.edu"
full_em="tfogal@sci.utah.edu jens@sci.utah.edu"

status="status-deb-vm"
function try
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." >> ${status}
        cat ${status} | mail -s "deb-vm nightly FAILED" ${em}
        exit 1
    fi
}

status="status-deb-vm"

echo "Using compiler version:" > ${status}
g++ --version >> ${status}
echo "" >> ${status}

echo "On system:" >> ${status}
uname -a >> ${status}
echo "" >> ${status}

echo "-------------------------------------" >> ${status}

try cd ${HOME}/imagevis3d
rm -f *.tar.gz *.zip warnings
try sh Scripts/nightly.sh
cat warnings >> ${status}
subj=""
if test `file warnings | awk '{print $2}'` = "empty" ; then
    subj="deb-vm nightly (clean) -- `date`"
else
    subj="deb-vm nightly (warnings) -- `date`"
fi
if test "$1" != "-q" ; then
    cat ${status} | mail -s "${subj}" ${full_em}
    devbuilds="/usr/sci/projects/sciweb/devbuilds/imagevis3d/"
    try scp *.tar.gz tfogal@shell.sci.utah.edu:${devbuilds}
fi
