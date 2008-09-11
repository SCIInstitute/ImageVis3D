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
  \file    VolumeDataset.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/


#pragma once

#ifndef VOLUMEDATASET_H
#define VOLUMEDATASET_H

#include <string>
#include "TransferFunction1D.h"
#include "TransferFunction2D.h"
#include <IO/UVF/UVF.h>


class VolumeDataset;
class MasterController;

// this is just a wrapper around the UVF data to hide the actuall implementation in case we want to replace it some time
class VolumeDatasetInfo {
  public:
    VolumeDatasetInfo() : m_pVolumeDataBlock(NULL) {}

  	const std::vector<UINT64>& GetBrickCount(const std::vector<UINT64>& vLOD) const {
      return m_pVolumeDataBlock->GetBrickCount(vLOD);
    }
    const std::vector<UINT64>& GetBrickSize(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) const {
      return m_pVolumeDataBlock->GetBrickSize(vLOD, vBrick);
    }
    const std::vector<UINT64> GetDomainSize() const {
      return m_pVolumeDataBlock->ulDomainSize;
    }
    const std::vector<UINT64> GetBrickSize() const {
      return m_pVolumeDataBlock->ulBrickSize;
    }
    const std::vector<UINT64> GetBrickOverlapSize() const {
      return m_pVolumeDataBlock->ulBrickOverlap;
    }    
    const std::vector<UINT64> GetLODLevelCount() const {
      return m_pVolumeDataBlock->ulLODLevelCount;
    }

    // TODO: change this if we want to support color data
    UINT64 GetBitwith() const {
      return m_pVolumeDataBlock->ulElementBitSize[0][0];
    }
    UINT64 GetComponentCount() const {
      return 1;
    }


  private:
    VolumeDatasetInfo(RasterDataBlock* pVolumeDataBlock) : m_pVolumeDataBlock(pVolumeDataBlock) {}
    RasterDataBlock*    m_pVolumeDataBlock;

    friend class VolumeDataset;
};

class VolumeDataset {
public:
  VolumeDataset(const std::string& strFilename, bool bVerify, MasterController* pMasterController);
  ~VolumeDataset();

  bool IsOpen() const {return m_bIsOpen;}
  std::string Filename() const {return m_strFilename;}

  const Histogram1D* Get1DHistogramm() const {return m_pHist1D;}
  const Histogram2D* Get2DHistogramm() const {return m_pHist2D;}

  const VolumeDatasetInfo* GetInfo() const {return m_pVolumeDatasetInfo;}

  UINTVECTOR3 GetBrickSize(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick);
  void GetBrickCenterAndExtension(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick, FLOATVECTOR3& vCenter, FLOATVECTOR3& vExtension);

private:
  MasterController*   m_pMasterController;
  RasterDataBlock*    m_pVolumeDataBlock;
  UVF*                m_pDatasetFile;

  bool                m_bIsOpen;
  std::string         m_strFilename;
  VolumeDatasetInfo*  m_pVolumeDatasetInfo;
  Histogram1D*        m_pHist1D;
  Histogram2D*        m_pHist2D;

  bool Open(bool bVerify);
  void ComputeHistogramms();

};

#endif // VOLUMEDATASET_H
