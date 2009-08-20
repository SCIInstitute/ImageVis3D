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
#include <QtGui/QApplication>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"
#include "DebugOut/HRConsoleOut.h"
#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/DirectoryParser.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;

#define CONV_VERSION 1.1

/*
#ifdef _WIN32
  // CRT's memory leak detection on windows
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif
  */

#ifndef TUVOK_NO_QT
  #include <QtGui/QImageReader>
#endif

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

int main(int argc, char* argv[])
{
/*
// Enable run-time memory check for debug builds on windows
  #ifdef _WIN32
    #if defined(DEBUG) | defined(_DEBUG)
      _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif
  #endif
*/

  // get command line paramers 
  SysTools::CmdLineParams parameters(argc, argv);
  string strFilename = SysTools::GetFilename(argv[0]);

  if (parameters.SwitchSet("?") || parameters.SwitchSet("HELP")) {
    ShowUsage(strFilename);
    return 1;   
  }

  string strInFile = "";
  string strInFile2 = "";
  string strInDir = "";
  string strOutfile = "";
  string strScale;
  double fScale = 1.0;
  string strBias = "";
  double fBias = 0.0;
  parameters.GetValue("D",strInDir);
  parameters.GetValue("F",strInFile);
  parameters.GetValue("F2",strInFile2);
  parameters.GetValue("B2",strBias);
  parameters.GetValue("S2",strScale);
  parameters.GetValue("OUT",strOutfile);

  // replace "m" by "-"
  string::size_type pos = 0;
  if (strScale != "") {
    while ( (pos = strScale.find("m", pos)) != string::npos ) {
      strScale.replace( pos, string("m").size(), "-" );
      pos++;
    }
    fScale = atof(strScale.c_str());
  }
  if (strBias != "") {
    pos = 0;
    while ( (pos = strBias.find("m", pos)) != string::npos ) {
      strBias.replace( pos, string("m").size(), "-" );
      pos++;
    }
    fBias = atof(strBias.c_str());
  }

  if (strInFile == "" && strInDir == "") {
    cout << "Must specify parameter either 'f' or parameter 'd'." << endl;
    ShowUsage(strFilename);
    return 1;
  }

  if (strInFile != "" && strInDir != "") {
    cout << "Must specify parameter either 'f' or parameter 'd' but not both." << endl;
    ShowUsage(strFilename);
    return 1;
  }


  HRConsoleOut* debugOut = new HRConsoleOut();
  debugOut->SetOutput(true, true, true, false);

  Controller::Instance().AddDebugOut(debugOut);
  IOManager ioMan;

  string targetType = SysTools::ToLowerCase(SysTools::GetExt(strOutfile));
  if (strInFile != "") {

    string sourceType = SysTools::ToLowerCase(SysTools::GetExt(strInFile));

    if (strInFile2 == "") {
      cout << "Running in file mode." << endl << "Converting " << strInFile.c_str() << " to " << strOutfile.c_str() << endl << endl;  
      if (ioMan.ConvertDataset(strInFile, strOutfile, SysTools::GetPath(strOutfile))) { // HACK: use the output file's dir as temp dir
        cout << "Success." << endl << endl;
        return 0;
      } else {
        cout << "Conversion failed!" << endl << endl;
        return 2;
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

      cout << "Running in merge mode." << endl << "Converting";
      for (size_t i = 0;i<<vDataSets.size();i++) {
        cout << " " << vDataSets[i];
      }        
      cout << " to " << strOutfile<< endl << endl;  

      if (ioMan.MergeDatasets(vDataSets, vScales, vBiases, strOutfile, SysTools::GetPath(strOutfile))) {  // HACK: use the output file's dir as temp dir
        cout << "Success." << endl << endl;
        return 0;
      } else {
        cout << "Merging datasets failed!" << endl << endl;
        return 2;
      }

    }

  } else {

    if (strInFile2 != "") {
      cout << "Error: Currently file merging is only supported in file mode (i.e. specify -f and not -d)." << endl << endl;
      return 2;
    }

    /// \todo: remove this restricition (one solution would be to create a UVF first and then convert it to whatever is needed)
    if (targetType != "uvf") {
      cout << "Error: Currently only uvf is only supported as target type for directory processing." << endl << endl;
      return 2;
    }

    cout << "Running in directory mode." << endl << "Converting " << strInDir.c_str() << " to " << strOutfile.c_str() << endl << endl;

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
        cout << "Success." << endl << endl;
      } else {
        cout << "Conversion failed!" << endl << endl;
        iFailCount++;
        for (size_t i = 0;i<dirinfo.size();i++) delete dirinfo[i];
        return 3;
      }
    }
    
    if (iFailCount != 0)  {
      cout << iFailCount << " out of " << dirinfo.size() << " stacks failed to convert properly."<< endl << endl;
    }

    for (size_t i = 0;i<dirinfo.size();i++) delete dirinfo[i];
    return 0;
  }
}
