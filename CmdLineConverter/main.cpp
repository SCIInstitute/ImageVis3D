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


//!    File   : main.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : December 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "../Tuvok/StdTuvokDefines.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <QtGui/QApplication>
#ifndef TUVOK_NO_QT
  #include <QtGui/QImageReader>
#endif
#include <tclap/CmdLine.h>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"
#include "DebugOut/HRConsoleOut.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/DirectoryParser.h"

using namespace std;
using namespace tuvok;

#define CONV_VERSION 1.1

/*
#ifdef _WIN32
  // CRT's memory leak detection on windows
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif
  */

void ShowUsage(string filename) {
  IOManager ioMan;

  std::vector< std::pair <std::string, std::string > > importList = ioMan.GetImportFormatList();
  std::vector< std::pair <std::string, std::string > > exportList = ioMan.GetExportFormatList();

    cout << endl <<
            filename << " V" << CONV_VERSION << " (using Tuvok V" << TUVOK_VERSION << " " << TUVOK_VERSION_TYPE << ")" << endl << endl <<
      " Converts different types of volumetric data." << endl << endl <<
      " Usage:" << endl <<
            "    " << filename << " -f InFile ^ -d InDir [-f2 InFile2] -out OutFile " << endl << endl <<
            "     Mandatory Arguments:" << endl <<
            "        -f    the input filename" << endl <<
      "          Supported formats: " << endl;
      for (size_t i = 0;i<importList.size();i++) {
        cout << "             " << importList[i].first.c_str() << " (" << importList[i].second.c_str() << ")" << endl;
      }
      cout <<
      "        XOR" << endl <<
            "        -d    the input directory" << endl <<
      "          Supported file formats in that directory: " << endl <<
      "             DICOM" << endl;
#ifndef TUVOK_NO_QT
      QList<QByteArray> listImageFormats = QImageReader::supportedImageFormats();
      for (size_t i = 0;i<exportList.size();i++) {
        QByteArray imageFormat = listImageFormats[int(i)];
        QString qStrImageFormat(imageFormat);
        string strImageFormat = qStrImageFormat.toStdString();
        cout << "             " << strImageFormat.c_str() << endl;
      }
#endif
      cout << endl <<
            "        -out  the target filename (the extension is used to detect the format)" << endl <<
      "          Supported formats: " << endl;
      for (size_t i = 0;i<exportList.size();i++) {
        cout << "             " << exportList[i].first.c_str() << " (" << exportList[i].second.c_str() << ")" << endl;
      }
      cout <<
            "     Optional Arguments:" << endl <<
            "        -f2   second input file to be merged with the first" << endl <<
            "        -b2   bias factor for values in the second input file" << endl <<
            "              (use 'm' instead of a minus sign e.g. -b2 m3 will bias by -3)" << endl <<
            "        -s2   scaling factor for values in the second input file" << endl <<
      "              (use 'm' instead of a minus sign e.g. -s2 m5 will scale by -5)" << endl <<
      " Examples:" << endl <<
      "  " << filename << " -f head.vff -out head.uvf   // converts head.vff" << endl <<
      "                                                    into a uvf file" << endl <<
      "  " << filename << " -f head.uvf -out head.nhdr  // converts head.uvf" << endl <<
      "                                                    into head.nhdr and" << endl <<
      "                                                    head.nhdr.raw" << endl <<
      "  " << filename << " -d .. -out data.uvf         // scanns the parent directory" << endl <<
      "                                                    for DICOM- and image-stacks" << endl <<
      "                                                    outputs data0.uvf to" << endl <<
      "                                                    dataN.uvf" << endl << endl;
}

int main(int argc, const char* argv[])
{
/*
// Enable run-time memory check for debug builds on windows
  #ifdef _WIN32
    #if defined(DEBUG) | defined(_DEBUG)
      _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif
  #endif
*/
  std::list<std::string> input;
  std::string output, directory;

  // temp
  string strInFile = "";
  string strInFile2 = "";
  string strInDir = "";
  string strOutfile = "";
  double fScale = 0.0;
  double fBias = 0.0;

  try {
    TCLAP::CmdLine cmd("test msg", ' ', "1.2.0");
    TCLAP::MultiArg<std::string> inputs("i", "input", "input file.  "
                                        "Repeat to merge volumes", true,
                                        "filename");
    TCLAP::ValueArg<std::string> directory("d", "directory",
                                           "input directory", true, "",
                                           "path");
    TCLAP::ValueArg<std::string> output("o", "output", "output file (uvf)",
                                        true, "", "filename");
    TCLAP::ValueArg<double> bias("b", "bias",
                                 "(merging) bias value for second file",
                                 false, 0.0, "floating point number");
    TCLAP::ValueArg<double> scale("s", "scale",
                                  "(merging) scaling value for second file",
                                  false, 0.0, "floating point number");
    cmd.xorAdd(inputs, directory);
    cmd.add(output);
    cmd.add(bias);
    cmd.add(scale);
    cmd.parse(argc, argv);

    // which of "-i" or "-d" did they give?
    if(inputs.isSet()) {
      strInFile = inputs.getValue()[0];
      if(inputs.getValue().size() > 1) {
        strInFile2 = inputs.getValue()[1];
      }
    }
    if(directory.isSet()) {
      strInDir = directory.getValue();
    }
    strOutfile = output.getValue();
    fBias = bias.getValue();
    fScale = scale.getValue();
  } catch(const TCLAP::ArgException& e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << "\n";
    return EXIT_FAILURE;
  }

  HRConsoleOut* debugOut = new HRConsoleOut();
  debugOut->SetOutput(true, true, true, false);
  debugOut->SetClearOldMessage(true);

  Controller::Instance().AddDebugOut(debugOut);
  IOManager ioMan;

  string targetType = SysTools::ToLowerCase(SysTools::GetExt(strOutfile));
  if (strInFile != "") {

    string sourceType = SysTools::ToLowerCase(SysTools::GetExt(strInFile));

    if (strInFile2 == "") {

      if (targetType == "uvf" && sourceType == "uvf") {
        cout << endl << "Running in UVF to UVF mode, perserving only the raw data from " << strInFile.c_str() << " to " << strOutfile.c_str() << endl;


        cout << "Step 1. Extracting raw data" << endl;
        string tmpFile = SysTools::ChangeExt(strOutfile,"nrrd"); /// use some simple format as intermediate file

        if (ioMan.ConvertDataset(strInFile, tmpFile, SysTools::GetPath(tmpFile))) { // HACK: use the output file's dir as temp dir
          cout << endl << "Success." << endl << endl;
        } else {
          cout << endl << "Extraction failed!" << endl << endl;
          return EXIT_FAILURE;
        }

        cout << "Step 2. Writing new UVF file" << endl;

        if (ioMan.ConvertDataset(tmpFile, strOutfile, SysTools::GetPath(strOutfile))) { // HACK: use the output file's dir as temp dir
          if(std::remove(tmpFile.c_str()) == -1) {
           cout << endl << "Conversion succeeded but could not delete tmp file " << tmpFile.c_str() << endl << endl;
          } else {
           cout << endl << "Success." << endl << endl;
          }
          return EXIT_SUCCESS;
        } else {
          if(std::remove(tmpFile.c_str()) == -1) {
           cout << endl << "UVF write failed and could not delete tmp file " << tmpFile.c_str() << endl << endl;
          } else {
           cout << endl << "UVF write failed." << endl << endl;
          }
          return EXIT_FAILURE;
        }



      } else {
        cout << endl << "Running in file mode." << endl << "Converting " << strInFile.c_str() << " to " << strOutfile.c_str() << endl << endl;
        if (ioMan.ConvertDataset(strInFile, strOutfile, SysTools::GetPath(strOutfile))) { // HACK: use the output file's dir as temp dir
          cout << endl << "Success." << endl << endl;
          return EXIT_SUCCESS;
        } else {
          cout << endl << "Conversion failed!" << endl << endl;
          return EXIT_FAILURE;
        }
      }
    } else {
      vector<string> vDataSets;
      vector<double> vScales;
      vector<double> vBiases;
      vDataSets.push_back(strInFile);
      vScales.push_back(1.0);
      vBiases.push_back(0.0);
      vDataSets.push_back(strInFile2);
      vScales.push_back(fScale);
      vBiases.push_back(fBias);

      cout << endl << "Running in merge mode." << endl << "Converting";
      for (size_t i = 0;i<<vDataSets.size();i++) {
        cout << " " << vDataSets[i];
      }
      cout << " to " << strOutfile<< endl << endl;

      if (ioMan.MergeDatasets(vDataSets, vScales, vBiases, strOutfile, SysTools::GetPath(strOutfile))) {  // HACK: use the output file's dir as temp dir
        cout << endl << "Success." << endl << endl;
        return EXIT_SUCCESS;
      } else {
        cout << endl << "Merging datasets failed!" << endl << endl;
        return EXIT_FAILURE;
      }

    }

  } else {

    if (strInFile2 != "") {
      cout << endl << "Error: Currently file merging is only supported in file mode (i.e. specify -f and not -d)." << endl << endl;
      return EXIT_FAILURE;
    }

    /// \todo: remove this restricition (one solution would be to create a UVF first and then convert it to whatever is needed)
    if (targetType != "uvf") {
      cout << endl << "Error: Currently only uvf is only supported as target type for directory processing." << endl << endl;
      return EXIT_FAILURE;
    }

    cout << endl << "Running in directory mode." << endl << "Converting " << strInDir.c_str() << " to " << strOutfile.c_str() << endl << endl;

    vector<FileStackInfo*> dirinfo = ioMan.ScanDirectory(strInDir);

    vector<string> vStrFilenames(dirinfo.size());
    if (dirinfo.size() == 1) {
       vStrFilenames[0] = strOutfile;
    } else {
      string strFilenameAndDirectory = SysTools::RemoveExt(strOutfile);
      string strExt = SysTools::GetExt(strOutfile);  // should be "uvf" but we never know what the user specified
      for (size_t i = 0;i<dirinfo.size();i++) {
        vStrFilenames[i] = SysTools::AppendFilename(strOutfile, int(i)+1);
      }
    }


    int iFailCount = 0;
    for (size_t i = 0;i<dirinfo.size();i++) {
      if (ioMan.ConvertDataset(dirinfo[i], vStrFilenames[i], SysTools::GetPath(vStrFilenames[i]))) {
        cout << endl << "Success." << endl << endl;
      } else {
        cout << endl << "Conversion failed!" << endl << endl;
        iFailCount++;
        for (size_t i = 0;i<dirinfo.size();i++) delete dirinfo[i];
        return EXIT_FAILURE;
      }
    }

    if (iFailCount != 0)  {
      cout << endl << iFailCount << " out of " << dirinfo.size() << " stacks failed to convert properly."<< endl << endl;
    }

    for (size_t i = 0;i<dirinfo.size();i++) delete dirinfo[i];
    return EXIT_SUCCESS;
  }
}
