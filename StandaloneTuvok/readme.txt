Compiling the Tuvok without ImageVis3D demo:

1) compile Tuvok with the TUVOK_NO_QT macro in StdTuvokDefines.h enabled
   making sure you use the same profile (DEBUG vs. RELEASE) as for this project
2) copy the library to this directory
3) copy the Tuvok Shaders directory to this directory
4) Build this Program
5) Run the program with -d "datafilename"
6) a single bmp image "image.bmp" will be created