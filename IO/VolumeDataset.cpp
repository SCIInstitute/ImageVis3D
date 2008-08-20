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


VolumeDataset::VolumeDataset(const std::string& strFilename) : 
  m_strFilename(strFilename),
  m_pVolumeDatasetInfo(NULL),
  m_pHist1D(NULL), 
  m_pHist2D(NULL)
{
  LoadDataset();
  ComputeHistogramms();
}

VolumeDataset::~VolumeDataset()
{
  delete m_pHist1D;
  delete m_pHist2D;
  delete m_pVolumeDatasetInfo;
}


bool VolumeDataset::IsLoaded() const
{
  // TODO
  return true;
}


void VolumeDataset::LoadDataset() 
{
  // TODO
  m_pVolumeDatasetInfo = new VolumeDatasetInfo(8);
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
