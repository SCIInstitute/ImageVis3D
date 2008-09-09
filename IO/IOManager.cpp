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
  \file    IOManager.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "IOManager.h"
#include <Controller/MasterController.h>
#include <IO/DICOM/DICOMParser.h>
#include <sstream>

using namespace std;

IOManager::IOManager(MasterController* masterController) :
  m_pMasterController(masterController)
{

}

IOManager::~IOManager()
{

}

vector<FileStackInfo*> IOManager::ScanDirectory(std::string strDirectory) {

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","Scanning directory %s", strDirectory.c_str());

  std::vector<FileStackInfo*> fileStacks;

  // right now we scan the directory only for DICOM files but in the future other image scanners will be added
  DICOMParser p;
  p.GetDirInfo(strDirectory);

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found %i DICOM stacks", p.m_FileStacks.size());

  for (unsigned int iStackID = 0;iStackID < p.m_FileStacks.size();iStackID++) {    
    FileStackInfo* f = new FileStackInfo(p.m_FileStacks[iStackID]);

    DICOMStackInfo* d = (DICOMStackInfo*)p.m_FileStacks[iStackID];

    stringstream s;
    s << "DICOM Stack: " << f->m_strDesc;
    f->m_strDesc = s.str();

    fileStacks.push_back(f);
  }

  // TODO: add other image parsers here

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  scan complete");

  return fileStacks;
}


bool IOManager::ConvertDataset(FileStackInfo* pStack, const std::string& strTargetFilename) {
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","Converting stack of images to %s", strTargetFilename.c_str());


  // TODO
  return true;
}

bool IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename) {
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","Converting stack dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());


  // TODO
  return true;
}

VolumeDataset* IOManager::ConvertDataset(FileStackInfo* pStack, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(pStack, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(strFilename, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::LoadDataset(std::string strFilename, AbstrRenderer* requester) {
  return m_pMasterController->MemMan()->LoadDataset(strFilename, requester);
}
