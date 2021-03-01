#!/bin/sh
cd ${HOME}/imagevis3d
source Scripts/util.sh

export PATH="${HOME}/sw/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"

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
touch warnings # we assume the file exists.
grep -v "IO/3rdParty" warnings > warn_no_3rd
mv warn_no_3rd warnings
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
devb_host="shell.sci.utah.edu"
mailtry scp *.tar.gz tfogal@${devb_host}:${devbuilds}

# Update `latest version' symlink.
fn_tarball=$(nm_tarball)
fn_tarball="${devbuilds}/${fn_tarball}"
fn_latest="${devbuilds}/ImageVis3D-Linux-Latest.tar.gz"
mailtry ssh tfogal@${devb_host} rm -f ${fn_latest}
mailtry ssh tfogal@${devb_host} ln -s ${fn_tarball} ${fn_latest}

# Update the text file for automagic version checks.
mailtry mv latest Linux_Latest_Version.txt
mailtry scp Linux_Latest_Version.txt tfogal@${devb_host}:${devbuilds}
