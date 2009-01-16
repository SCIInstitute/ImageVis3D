#!/bin/sh
source Scripts/util.sh

export PATH="${HOME}/sw/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"
# this affects what BSD mail uses for the Reply-To header:
export REPLYTO="tfogal@sci.utah.edu"

em="tfogal@sci.utah.edu"
full_em="tfogal@sci.utah.edu jens@sci.utah.edu"

status="status-deb-vm"
function mailtry
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." >> ${status}
        cat ${status} | mail -s "deb-vm nightly FAILED" ${em}
        exit 1
    fi
}

rm -f ${status} warnings
echo "Using compiler version:" > ${status}
g++ --version >> ${status}
echo "" >> ${status}

echo "On system:" >> ${status}
uname -a >> ${status}
echo "" >> ${status}

echo "-------------------------------------" >> ${status}

mailtry cd ${HOME}/imagevis3d
rm -f *.tar.gz *.zip warnings
mailtry sh Scripts/nightly.sh
cat warnings >> ${status}
subj=""
if test `file warnings | awk '{print $2}'` = "empty" ; then
    subj="deb-vm nightly (clean) -- `date`"
else
    subj="deb-vm nightly (warnings) -- `date`"
fi
if test "$1" != "-q" ; then
    cat ${status} | mail -s "${subj}" ${full_em}
fi

devbuilds="/usr/sci/projects/sciweb/devbuilds/imagevis3d"
mailtry scp *.tar.gz tfogal@shell.sci.utah.edu:${devbuilds}

# Update `latest version' symlink.
fn_tarball=$(nm_tarball)
fn_tarball="${devbuilds}/${fn_tarball}"
fn_latest="${devbuilds}/ImageVis3D-Linux-Latest.tar.gz"
mailtry ssh tfogal@shell.sci.utah.edu rm -f ${fn_latest}
mailtry ssh tfogal@shell.sci.utah.edu ln -s ${fn_tarball} ${fn_latest}
