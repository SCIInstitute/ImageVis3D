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
  \file    VolumeDataset.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "VolumeDataset.h"
#include <Controller/MasterController.h>
#include <cstdlib> 
#include <string> 

using namespace std;

VolumeDataset::VolumeDataset(const std::string& strFilename, bool bVerify, MasterController* pMasterController) : 
  m_pMasterController(pMasterController),
  m_pVolumeDataBlock(NULL),
  m_pDatasetFile(NULL),
  m_bIsOpen(false),
  m_strFilename(strFilename),
  m_pVolumeDatasetInfo(NULL),
  m_pHist1D(NULL), 
  m_pHist2D(NULL)
{
  Open(bVerify);
  ComputeHistogramms();
}

VolumeDataset::~VolumeDataset()
{
  delete m_pHist1D;
  delete m_pHist2D;
  delete m_pVolumeDatasetInfo;


  if (m_pDatasetFile != NULL) {
     m_pDatasetFile->Close();
    delete m_pDatasetFile;
  }
}



bool VolumeDataset::Open(bool bVerify) 
{
  wstring wstrFilename(m_strFilename.begin(),m_strFilename.end());
  m_pDatasetFile = new UVF(wstrFilename);
  m_bIsOpen = m_pDatasetFile->Open(bVerify);

  if (!m_bIsOpen) return false;

  UINT64 iRasterBlockIndex = UINT64(-1);
  for (size_t iBlocks = 0;iBlocks<m_pDatasetFile->GetDataBlockCount();iBlocks++) {
    if (m_pDatasetFile->GetDataBlock(iBlocks)->GetBlockSemantic() == UVFTables::BS_REG_NDIM_GRID) {
      RasterDataBlock* pVolumeDataBlock = (RasterDataBlock*)m_pDatasetFile->GetDataBlock(iBlocks);

      // check if the block is at least 3 dimensional
      if (pVolumeDataBlock->ulDomainSize.size() < 3) {
        m_pMasterController->DebugOut()->Message("VolumeDataset::Open","%i-D raster data block found in UVF file, skipping.", pVolumeDataBlock->ulDomainSize.size());
        continue;
      }

      // check if the ulElementDimension = 1 e.g. we can deal with scalars and vectors
      if (pVolumeDataBlock->ulElementDimension != 1) {
        m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Non scalar/vector raster data block found in UVF file, skipping.");
        continue;
      }

      // TODO: rethink this for time dependent data
      if (pVolumeDataBlock->ulLODGroups[0] != pVolumeDataBlock->ulLODGroups[1] || pVolumeDataBlock->ulLODGroups[1] != pVolumeDataBlock->ulLODGroups[2]) {
        m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Raster data block with unsupported LOD layout found in UVF file, skipping.");
        continue;
      }      

      // TODO: change this if we want to support color/vector data
      // check if we have anything other than scalars 
      if (pVolumeDataBlock->ulElementDimensionSize[0] != 1) {
        m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Non scalar raster data block found in UVF file, skipping.");
        continue;
      }

      if (iRasterBlockIndex != UINT64(-1)) {
        m_pMasterController->DebugOut()->Warning("VolumeDataset::Open","Multiple volume blocks found using last block.");
      }
      iRasterBlockIndex = iBlocks;
    } else {
      m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Non-volume block found in UVF file, skipping.");
    }
  }

  if (iRasterBlockIndex == UINT64(-1)) {
    m_pMasterController->DebugOut()->Error("VolumeDataset::Open","No suitable volume block found in UVF file. Check previous messages for rejected blocks.");
    return false;
  }

  m_pVolumeDataBlock = (RasterDataBlock*)m_pDatasetFile->GetDataBlock(iRasterBlockIndex);

  m_pVolumeDatasetInfo = new VolumeDatasetInfo();

  m_pVolumeDatasetInfo->m_ulDomainSize    = m_pVolumeDataBlock->ulDomainSize;
  m_pVolumeDatasetInfo->m_ulBrickSize     = m_pVolumeDataBlock->ulBrickSize;
	m_pVolumeDatasetInfo->m_ulBrickOverlap  = m_pVolumeDataBlock->ulBrickOverlap;
  m_pVolumeDatasetInfo->m_ulLODLevelCount = m_pVolumeDataBlock->ulLODLevelCount[0];  // we checked above that ulLODGroups[0..2] are all the same

  // TODO: change this if we want to support color data
  m_pVolumeDatasetInfo->m_ulBitwith        = m_pVolumeDataBlock->ulElementBitSize[0][0];
  m_pVolumeDatasetInfo->m_ulComponentCount = 1;

  // TODO: transfer brick layout per LOD level 


  return true;
}

void VolumeDataset::ComputeHistogramms() 
{
  // DEBUG CODE
  // generate a random 1D histogram
  m_pHist1D = new Histogram1D(1<<m_pVolumeDatasetInfo->GetBitwith());
  for (size_t i = 0;i<m_pHist1D->GetSize();i++) m_pHist1D->Set(i, (unsigned int)(i+(rand()*10) / RAND_MAX));
  // generate a random 2D histogram
  m_pHist2D = new Histogram2D(VECTOR2<size_t>(1<<m_pVolumeDatasetInfo->GetBitwith(),256));  // TODO: decide: always 8bit gradient ?
  for (size_t y = 0;y<m_pHist2D->GetSize().y;y++)
    for (size_t x = 0;x<m_pHist2D->GetSize().x;x++) 
      m_pHist2D->Set(x,y,(unsigned int)(x+(rand()*10) / RAND_MAX)*(unsigned int)(y+(rand()*10) / RAND_MAX));
  // END DEBUG CODE
}
