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
#include "IOManager.h"  // for the size defines
#include <Controller/MasterController.h>
#include <cstdlib> 
#include <string>
#include <sstream>

using namespace std;

VolumeDataset::VolumeDataset(const std::string& strFilename, bool bVerify, MasterController* pMasterController) : 
  m_pMasterController(pMasterController),
  m_pVolumeDataBlock(NULL),
  m_pHist1DDataBlock(NULL),
  m_pHist2DDataBlock(NULL),
  m_pDatasetFile(NULL),
  m_bIsOpen(false),
  m_strFilename(strFilename),
  m_pVolumeDatasetInfo(NULL),
  m_pHist1D(NULL), 
  m_pHist2D(NULL)
{
  Open(bVerify);
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
    if (m_pDatasetFile->GetDataBlock(iBlocks)->GetBlockSemantic() == UVFTables::BS_1D_Histogram) {
      if (m_pHist1DDataBlock != NULL) {
        m_pMasterController->DebugOut()->Warning("VolumeDataset::Open","Multiple 1D Histograms found using last block.");
      }
      m_pHist1DDataBlock = (Histogram1DDataBlock*)m_pDatasetFile->GetDataBlock(iBlocks);
    } else
    if (m_pDatasetFile->GetDataBlock(iBlocks)->GetBlockSemantic() == UVFTables::BS_2D_Histogram) {
      if (m_pHist2DDataBlock != NULL) {
        m_pMasterController->DebugOut()->Warning("VolumeDataset::Open","Multiple 2D Histograms found using last block.");
      }
      m_pHist2DDataBlock = (Histogram2DDataBlock*)m_pDatasetFile->GetDataBlock(iBlocks);
    } else
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

      // check if the data's smallest LOD level is not larger than our bricksize
      // TODO: if this fails we may want to convert the dataset
      std::vector<UINT64> vSmallLODBrick = pVolumeDataBlock->GetSmallestBrickSize();
      bool bToFewLODLevels = false;
      for (size_t i = 0;i<vSmallLODBrick.size();i++) {
        if (vSmallLODBrick[i] > BRICKSIZE) {
          m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Raster data block with insufficient LOD levels found in UVF file, skipping.");
          bToFewLODLevels = true;
          break;
        }
      }
      if (bToFewLODLevels) continue;

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

  m_pMasterController->DebugOut()->Message("VolumeDataset::Open","Open successfully found a suitable data block in the UVF file, analysing data...");

  m_pVolumeDataBlock = (RasterDataBlock*)m_pDatasetFile->GetDataBlock(iRasterBlockIndex);
  m_pVolumeDatasetInfo = new VolumeDatasetInfo(m_pVolumeDataBlock);

  stringstream sStreamDomain, sStreamBrick;

  for (size_t i = 0;i<m_pVolumeDatasetInfo->GetDomainSize().size();i++) {
    if (i == 0)
      sStreamDomain << m_pVolumeDatasetInfo->GetDomainSize()[i];
    else
      sStreamDomain << " x " << m_pVolumeDatasetInfo->GetDomainSize()[i];
  }

  std::vector<UINT64> vSmallLODBrick = m_pVolumeDataBlock->GetSmallestBrickSize();
  for (size_t i = 0;i<vSmallLODBrick.size();i++) {
    if (i == 0)
      sStreamBrick << vSmallLODBrick[i];
    else
      sStreamBrick << " x " << vSmallLODBrick[i];
  }

  m_pHist1D = NULL;
  if (m_pHist1DDataBlock != NULL) {
    const vector<UINT64>& vHist1D = m_pHist1DDataBlock->GetHistogram();
    m_pHist1D = new Histogram1D(vHist1D.size());
    for (size_t i = 0;i<m_pHist1D->GetSize();i++) {
      m_pHist1D->Set(i, (unsigned int)vHist1D[i]);
    }
  } else {
    // generate a zero 1D histogram (max 4k) if none is found in the file
    m_pHist1D = new Histogram1D(min(4096, 1<<m_pVolumeDatasetInfo->GetBitwith()));
    for (size_t i = 0;i<m_pHist1D->GetSize();i++) {
      m_pHist1D->Set(i, 0);
    }
  }

  m_pHist2D = NULL;
  if (m_pHist2DDataBlock != NULL) {
    const vector< vector<UINT64> >& vHist2D = m_pHist2DDataBlock->GetHistogram();

    VECTOR2<size_t> vSize(vHist2D.size(),vHist2D[0].size());

    m_pHist2D = new Histogram2D(vSize);
    for (size_t y = 0;y<m_pHist2D->GetSize().y;y++)
      for (size_t x = 0;x<m_pHist2D->GetSize().x;x++) 
        m_pHist2D->Set(x,y,(unsigned int)vHist2D[x][y]);

  } else {
    // generate a zero 2D histogram (max 4k) if none is found in the file
    m_pHist2D = new Histogram2D(VECTOR2<size_t>(256,min(4096, 1<<m_pVolumeDatasetInfo->GetBitwith())));
    for (size_t y = 0;y<m_pHist2D->GetSize().y;y++)
      for (size_t x = 0;x<m_pHist2D->GetSize().x;x++) 
        m_pHist2D->Set(x,y,0);
  }

  m_pMasterController->DebugOut()->Message("VolumeDataset::Open","  Size %s", sStreamDomain.str().c_str());
  m_pMasterController->DebugOut()->Message("VolumeDataset::Open","  %i Bit, %i components", m_pVolumeDatasetInfo->GetBitwith(), m_pVolumeDatasetInfo->GetComponentCount());
  m_pMasterController->DebugOut()->Message("VolumeDataset::Open","  LOD down to %s found", sStreamBrick.str().c_str());

  return true;
}



UINTVECTOR3 VolumeDataset::GetBrickSize(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) {
  UINTVECTOR3 vSize;
  vector<UINT64> vSizeUVF = m_pVolumeDatasetInfo->GetBrickSize(vLOD, vBrick);

  // TODO: this code assumes that x,y,z are the first coords in the dataset which does not have to be, so better check this at load time
  vSize[0] = (unsigned int)(vSizeUVF[0]);
  vSize[1] = (unsigned int)(vSizeUVF[1]);
  vSize[2] = (unsigned int)(vSizeUVF[2]);

  return vSize;
}

void VolumeDataset::GetBrickCenterAndExtension(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick, FLOATVECTOR3& vCenter, FLOATVECTOR3& vExtension) {
  vector<UINT64> iBrickCount = m_pVolumeDatasetInfo->GetBrickCount(vLOD);

  // TODO
  vCenter[0] = 0;
  vCenter[1] = 0;
  vCenter[2] = 0;

  vExtension[0] = 1;
  vExtension[1] = 1;
  vExtension[2] = 1;
}
