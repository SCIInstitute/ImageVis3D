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
  \file    DialogConverter.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    December 2008
*/

#include "DialogConverter.h"
#include "../UI/RAWDialog.h"
#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Basics/LargeRAWFile.h"

#include <QtGui/QMessageBox>

using namespace std;

DialogConverter::DialogConverter(QWidget* parent) :
  m_parent(parent)
{
}

bool DialogConverter::ConvertToRAW(const std::string& strSourceFilename, 
                                   const std::string& strTempDir, bool bNoUserInteraction,
                                   UINT64& iHeaderSkip, UINT64& iComponentSize, UINT64& iComponentCount, 
                                   bool& bConvertEndianess, bool& bSigned, bool& bIsFloat, UINT64VECTOR3& vVolumeSize,
                                   FLOATVECTOR3& vVolumeAspect, std::string& strTitle,
                                   UVFTables::ElementSemanticTable& eType, std::string& strIntermediateFile,
                                   bool& bDeleteIntermediateFile) {

  if (bNoUserInteraction) return false;

  if (QMessageBox::No == QMessageBox::question(NULL, "RAW Data Loader", "The file was not recognized by ImageVis3D's built-in readers and cannot be converted automatically. Do you want to specify the data set parameters manually?", QMessageBox::Yes, QMessageBox::No)) {
    return false;
  }
  
  LargeRAWFile f(strSourceFilename);
  f.Open(false);
  UINT64 iSize = f.GetCurrentSize();
  f.Close();

  RAWDialog rawDialog(strSourceFilename, iSize, m_parent);
  if (rawDialog.exec() == QDialog::Accepted) {

    if (rawDialog.ComputeExpectedSize() > iSize) return false;


    strTitle = "Raw data";
    eType             = UVFTables::ES_UNDEFINED;
    iComponentCount = 1; 
    vVolumeSize    = rawDialog.GetSize();
    vVolumeAspect  = rawDialog.GetAspectRatio();
    unsigned int  quantID        = rawDialog.GetQuantization();
    unsigned int  encID          = rawDialog.GetEncoding();
    iHeaderSkip       = rawDialog.GetHeaderSize();
    bConvertEndianess = encID != 1 && rawDialog.IsBigEndian() != EndianConvert::IsBigEndian();
    bSigned           = quantID == 2 || rawDialog.IsSigned();
    

    iComponentSize = 8;
    if (quantID == 1) iComponentSize = 16;
      if (quantID > 1) iComponentSize = 32;

    bIsFloat = quantID == 3;

    if (encID == 0)  {
      strIntermediateFile = strSourceFilename;
      bDeleteIntermediateFile = false;
      return true;
    } else
    if (encID == 1)  {
        string strBinaryFile = strTempDir+SysTools::GetFilename(strSourceFilename)+".binary";
        bool bResult = ParseTXTDataset(strSourceFilename, strBinaryFile, iHeaderSkip, iComponentSize, iComponentCount, bSigned, bIsFloat, vVolumeSize);
        strIntermediateFile = strBinaryFile;
        bDeleteIntermediateFile = true;
        iHeaderSkip = 0;
        bConvertEndianess = false;
        return bResult;
    } else
    if (encID == 2)  {
        string strUncompressedFile = strTempDir+SysTools::GetFilename(strSourceFilename)+".uncompressed";
        bool bResult = ExtractGZIPDataset(strSourceFilename, strUncompressedFile, iHeaderSkip);
        strIntermediateFile = strUncompressedFile;
        bDeleteIntermediateFile = true;
        iHeaderSkip = 0;
        return bResult;
    } else {
        string strUncompressedFile = strTempDir+SysTools::GetFilename(strSourceFilename)+".uncompressed";
        bool bResult = ExtractBZIP2Dataset(strSourceFilename, strUncompressedFile, iHeaderSkip);
        strIntermediateFile = strUncompressedFile;
        bDeleteIntermediateFile = true;
        iHeaderSkip = 0;
        return bResult;
    } 
  }

  return false;
}
