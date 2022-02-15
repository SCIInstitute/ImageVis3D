#!/bin/sh
cd ${HOME}/imagevis3d
source Scripts/util.sh

export PATH="${HOME}/sw/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"
# this affects what BSD mail uses for the Reply-To header:
export REPLYTO="tfogal@sci.utah.edu"

# if something should fail:
em="tfogal@sci.utah.edu"
# warnings and other information:
full_em="tfogal@sci.utah.edu"

status="status-neon"
# like 'try' but sends an email if it fails.
function mailtry
{
    $@
    if test $? -ne 0 ; then
        echo "'$@' failed, bailing .."
        echo "Command: '$@' failed..." >> ${status}
        cat ${status} | mail -s "Neon nightly FAILED" ${em}
        exit 1
    fi
}

echo "Using compiler version:" > ${status}
g++ --version >> ${status}
echo "" >> ${status}

echo "On system:" >> ${status}
uname -a >> ${status}
echo "" >> ${status}

echo "-------------------------------------" >> ${status}

try cd ${HOME}/imagevis3d
rm -f *.tar.gz *.zip *.dmg warnings

mailtry sh Scripts/nightly.sh
grep -v "IO/3rdParty" warnings > warn_no_3rd
mv warn_no_3rd warnings
cat warnings >> ${status}
subj=""
if test `file warnings | awk '{print $2}'` = "empty" ; then
    subj="Neon nightly (clean) -- `date`"
else
    subj="Neon nightly (warnings) -- `date`"
fi
if test "$1" != "-q" ; then
    cat ${status} | mail -s "${subj}" ${full_em}
fi
devb="/usr/sci/projects/sciweb/devbuilds/imagevis3d/"
mailtry scp *.zip tfogal@shell.sci.utah.edu:${devb}
mailtry scp *.dmg tfogal@shell.sci.utah.edu:${devb}

# Now update the `latest version' symlink.
fn_zip=$(nm_zipfile)
fn_zip="${devb}/${fn_zip}"
fn_latest="${devb}/ImageVis3D-OSX-10.5-Latest.zip"
mailtry ssh tfogal@shell.sci.utah.edu rm -f ${fn_latest}
mailtry ssh tfogal@shell.sci.utah.edu rm -f ${fn_latest%%zip}dmg
mailtry ssh tfogal@shell.sci.utah.edu ln -s ${fn_zip} ${fn_latest}
mailtry ssh tfogal@shell.sci.utah.edu ln -s ${fn_zip%%zip}dmg \
                                            ${fn_latest%%zip}dmg

# Update the text file for determining the latest version.
mailtry mv latest OSX_Latest_Version.txt
mailtry scp OSX_Latest_Version.txt tfogal@shell.sci.utah.edu:${devb}
