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
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute


#include "Basics/StdDefines.h"
#include "StdTuvokDefines.h"
#include "Basics/SysTools.h"
#include "Basics/Appendix.h"

#include <iostream>
using namespace std;

constexpr auto EXTRACTOR_VERSION = 1.0;

/*
#ifdef _WIN32
  // CRT's memory leak detection on windows
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif
  */

void ShowUsage(string filename) {
	cout << endl <<
			filename << " V" << EXTRACTOR_VERSION << " (using Tuvok V" << TUVOK_VERSION << TUVOK_VERSION_TYPE << ")"<< endl << "  (c) Scientific Computing and Imaging Institute, University of Utah" << endl << endl <<
      " Usage:" << endl <<
			"    " << filename << " -f InFile [-x ^ -l] [-d OutDir]" << endl << endl <<		
			"     Mandatory Arguments:" << endl <<
			"        -f   the input filename" << endl << 
			"        -l   list the contents of the file" << endl << 
			"        XOR" << endl << 
			"        -x   extract the file" << endl << 
			"     Optional Arguments:" << endl <<
			"        -d   the target directory" << endl << endl;
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
  string strOutDir = "";
  parameters.GetValue("F",strInFile);
  parameters.GetValue("D",strOutDir);

  if (strInFile == "") {
    cout << "Must specify input file via parameter 'f'." << endl;
    ShowUsage(strFilename);
    return 1;
  }

  if (!parameters.SwitchSet("x") && !parameters.SwitchSet("l")) {
    cout << "Must specify parameter either 'x' or parameter 'l'." << endl;
    ShowUsage(strFilename);
    return 1;
  }

  if (parameters.SwitchSet("x") && parameters.SwitchSet("l")) {
    cout << "Must specify parameter either 'x' or parameter 'l' but not both." << endl;
    ShowUsage(strFilename);
    return 1;
  }

  if (!SysTools::FileExists(strInFile)) {
    cout << "Input file " << strInFile << " not found" << endl;
    return 2;
  }

  Appendix a(strInFile);
  if (!a.IsOK()) {
    cout << "Invalid input file." << endl;
    return 3;
  }
  const vector<FileInfo> fi = a.ListFiles();
  
  if (parameters.SwitchSet("l")) {
    for (size_t i = 0;i<fi.size();i++) {

      if (fi[i].m_iSize < 1024 *10) {
        cout << "  " << fi[i].m_strName << " Size: " << fi[i].m_iSize << " byte" << endl;
      } else {
        if (fi[i].m_iSize < 1024 *1024*10) {
          cout << "  " << fi[i].m_strName << " Size: " << fi[i].m_iSize/1024 << " KB" << endl;
        } else {
          if (fi[i].m_iSize < uint64_t(1024)*1024*1024*10) {
            cout << "  " << fi[i].m_strName << " Size: " << fi[i].m_iSize/(1024*1024) << " MB" << endl;
          } else {
            cout << "  " << fi[i].m_strName << " Size: " << fi[i].m_iSize/(uint64_t(1024)*1024*1024) << " TB" << endl;
          }
        }
      }
    }
    cout << endl;
  } else {
  
    if (strOutDir != "" && (strOutDir[strOutDir.length()-1] != '/' || strOutDir[strOutDir.length()-1] != '\\')) {
        strOutDir =  strOutDir + '/';
    }
    
    for (size_t i = 0;i<fi.size();i++) {
      cout << "Extracting: " << fi[i].m_strName << endl;
      a.ExtractFile(i,strOutDir + fi[i].m_strName);
    }
    cout << endl;
  }

  return 0;
}
