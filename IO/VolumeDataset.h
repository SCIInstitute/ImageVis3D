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

class VolumeDatasetInfo {
  public:
    VolumeDatasetInfo() {}

    UINT64 GetBitwith() const {return m_ulBitwith;}
    UINT64 GetComponentCount() const {return m_ulComponentCount;}

  private:
    std::vector<UINT64> m_ulDomainSize;
	  std::vector<UINT64> m_ulBrickSize;
	  std::vector<UINT64> m_ulBrickOverlap;
	  UINT64 m_ulLODLevelCount;
  	UINT64 m_ulBitwith;
    UINT64 m_ulComponentCount;

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
