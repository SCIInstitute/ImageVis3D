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
#include "../Tuvok/IO/UVF/LargeRAWFile.h"

using namespace std;


DialogConverter::DialogConverter(QWidget* parent) :
  m_parent(parent)
{
}

bool DialogConverter::Convert(const std::string& strSourceFilename, const std::string& strTargetFilename, const std::string& strTempDir, MasterController* pMasterController)
{
  pMasterController->DebugOut()->Message("DialogConverter::Convert","Attempting to interactively convert dataset %s to %s", strSourceFilename.c_str(), strTargetFilename.c_str());
  
  LargeRAWFile f(strSourceFilename);
  f.Open(false);
  UINT64 iSize = f.GetCurrentSize();
  f.Close();

  RAWDialog rawDialog(strSourceFilename, iSize, m_parent);
  if (rawDialog.exec() == QDialog::Accepted) {

    if (rawDialog.ComputeExpectedSize() > iSize) return false;

    UINTVECTOR3   vVolumeSize    = rawDialog.GetSize();
    FLOATVECTOR3  vVolumeAspect  = rawDialog.GetAspectRatio();
    unsigned int  quantID        = rawDialog.GetQuantization();
    unsigned int  encID          = rawDialog.GetEncoding();
    unsigned int  iHeaderSkip    = rawDialog.GetHeaderSize();
    bool          bConvEndian    = encID != 1 && rawDialog.IsBigEndian() != EndianConvert::IsBigEndian();
    bool          bSigned        = quantID == 2 || rawDialog.IsSigned();
    

    unsigned int iComponentSize = 8;
    if (quantID == 1) iComponentSize = 16;
      if (quantID == 2) iComponentSize = 32;

    if (encID == 0)  {
      return ConvertRAWDataset(strSourceFilename, strTargetFilename, strTempDir, pMasterController, iHeaderSkip, iComponentSize, 1, bConvEndian, bSigned, 
                               vVolumeSize, vVolumeAspect, "Raw data", SysTools::GetFilename(strSourceFilename));
    } else
    if (encID == 1)  {
      return ConvertTXTDataset(strSourceFilename, strTargetFilename, strTempDir, pMasterController, iHeaderSkip, iComponentSize, 1, bSigned,
                               vVolumeSize, vVolumeAspect, "Raw data", SysTools::GetFilename(strSourceFilename));
    } else
    if (encID == 2)  {
      return ConvertGZIPDataset(strSourceFilename, strTargetFilename, strTempDir, pMasterController, iHeaderSkip, iComponentSize, 1, bConvEndian, bSigned, 
                               vVolumeSize, vVolumeAspect, "Raw data", SysTools::GetFilename(strSourceFilename));

    } else {
      return ConvertBZIP2Dataset(strSourceFilename, strTargetFilename, strTempDir, pMasterController, iHeaderSkip, iComponentSize, 1, bConvEndian, bSigned,
                                 vVolumeSize, vVolumeAspect, "Raw data", SysTools::GetFilename(strSourceFilename));

    } 
  }

  return false;
}
