#pragma once

#ifndef STDIMAGEVISDEFINES_H
#define STDIMAGEVISDEFINES_H

#include "../Tuvok/StdTuvokDefines.h"

#define IV3D_MAJOR 4
#define IV3D_MINOR 0
#define IV3D_PATCH 0

#define IV3D_VERSION "4.0.0"
#define IV3D_VERSION_TYPE "Release Version"

#define MANUAL_NAME  "ImageVis3D.pdf"
#define HELP_URL  "http://hpc.uni-due.de/data/IV3D/ImageVis3D.pdf"
#define TUTORIAL_URL "http://www.imagevis3d.org/"
#define DEMO_DATA_URL  "http://www.sci.utah.edu/download/IV3DData.html"
#define DEBUG_DUMP_SERVER   L"ftp://ftp.sci.utah.edu"
#define DEBUG_DUMP_PATH     L"upload/ImageVis3D/"
//#define UPDATE_VERSION_PATH "http://www.sci.utah.edu/devbuilds/imagevis3d/"
//#define UPDATE_NIGHTLY_PATH "http://www.sci.utah.edu/devbuilds/imagevis3d/
#define UPDATE_VERSION_PATH "http://hpc.uni-due.de/software/stable/"
#define UPDATE_NIGHTLY_PATH "http://hpc.uni-due.de/software/devbuild/"

#define UPDATE_STABLE_PATH  "http://www.sci.utah.edu/download/imagevis3d.html"


#define SCI_ORGANIZATION_DOMAIN "http://software.sci.utah.edu/"
#ifdef DETECTED_OS_WINDOWS
  #define UPDATE_FILE "ImageVis3D-Windows-Latest.zip"
  #define UPDATE_VERSION_FILE "ImageVis3D-Windows_Latest_Version.txt"
#endif

#ifdef DETECTED_OS_APPLE
  #define UPDATE_FILE "ImageVis3D-OSX-Latest.zip"
  #define UPDATE_VERSION_FILE "ImageVis3D-OSX_Latest_Version.txt"
#endif

#ifdef DETECTED_OS_LINUX
  #define UPDATE_FILE "ImageVis3D-Linux-Latest.zip"
  #define UPDATE_VERSION_FILE "ImageVis3D-Linux_Latest_Version.txt"
#endif

// make sure they have a definition, so code that uses the defines always
// compiles... even if that definition doesn't exactly make sense.
#ifndef UPDATE_FILE
# define UPDATE_FILE "ImageVis3D-Unsupported-Latest.zip"
#endif
#ifndef UPDATE_VERSION_FILE
# define UPDATE_VERSION_FILE "Unsupported-Latest-Version.txt"
#endif

#endif // STDIMAGEVISDEFINES_H

/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/**
  \file    StdDefines.h
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \date    October 2008
*/
