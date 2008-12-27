
set path=%path%;C:\Program Files (x86)\CollabNet Subversion

cd..
svn up

svn info > rev1.txt
cd Tuvok
svn info > ..\rev2.txt
cd ..

setx IV3DVERSION /F rev1.txt /R 0,1 Revision: 
setx TUVOKVERSION /F rev2.txt /R 0,1 Revision: 
setx IV3DCODEVERSION /F ImageVis3D\StdDefines.h /R 0,1 IV3D_VERSION 

del rev1.txt
del rev2.txt