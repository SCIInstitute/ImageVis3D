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
  \file    TransferFunction2D.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    September 2008
*/

#include "TransferFunction2D.h"
#include <memory.h>

using namespace std;

TransferFunction2D::TransferFunction2D() :
  m_iSize(0,0)
{
  Resize(m_iSize);
}

TransferFunction2D::TransferFunction2D(const std::string& filename):
  m_iSize(0,0)
{
  Load(filename);
}

TransferFunction2D::TransferFunction2D(const VECTOR2<size_t>& iSize):
  m_iSize(iSize)
{
  Resize(m_iSize);
}

TransferFunction2D::~TransferFunction2D(void)
{
}

void TransferFunction2D::Resize(const VECTOR2<size_t>& iSize) {
  m_iSize = iSize;
  m_Trans1D.Resize(iSize.x);
  m_Trans1D.Clear();
}


bool TransferFunction2D::Load(const std::string& filename) {
  ifstream file(filename.c_str());

  if (!file.is_open()) return false;

  // load size
  file >> m_iSize.x >> m_iSize.y;

  // load 1D Trans
  m_Trans1D.Load(file, m_iSize.x);

  // load swatch count
  unsigned int iSwatchCount;
  file >> iSwatchCount;
  m_Swatches.resize(iSwatchCount);

  // load Swatches
  for (unsigned int i = 0;i<m_Swatches.size();i++) m_Swatches[i].Load(file);

  file.close();

  return true;
}

bool TransferFunction2D::Save(const std::string& filename) {
  ofstream file(filename.c_str());

  if (!file.is_open()) return false;

  // save size
  file << m_iSize.x << " " << m_iSize.y << endl;

  // save 1D Trans
  m_Trans1D.Save(file);

  // save swatch count
  file << m_Swatches.size() << endl;

  // save Swatches
  for (unsigned int i = 0;i<m_Swatches.size();i++) m_Swatches[i].Save(file);

  file.close();

  return true;
}

void TransferFunction2D::GetByteArray(unsigned char** pcData, unsigned char cUsedRange) {
  if (*pcData == NULL) *pcData = new unsigned char[m_iSize.area()*4];

  ColorData2D* pColorData = RenderTransferFunction();
  unsigned char *pcDataIterator = *pcData;
  FLOATVECTOR4  *piSourceIterator = pColorData->GetDataPointer();
  for (unsigned int i = 0;i<pColorData->GetSize().area();i++) {
    *pcDataIterator++ = (unsigned char)((*piSourceIterator)[0]*cUsedRange);
    *pcDataIterator++ = (unsigned char)((*piSourceIterator)[1]*cUsedRange);
    *pcDataIterator++ = (unsigned char)((*piSourceIterator)[2]*cUsedRange);
    *pcDataIterator++ = (unsigned char)((*piSourceIterator)[3]*cUsedRange);
    piSourceIterator++;
  }
  delete pColorData;
}

void TransferFunction2D::GetShortArray(unsigned short** psData, unsigned short sUsedRange) {
  if (*psData == NULL) *psData = new unsigned short[m_iSize.area()*4];

  ColorData2D* pColorData = RenderTransferFunction();
  unsigned short *psDataIterator = *psData;
  FLOATVECTOR4  *piSourceIterator = pColorData->GetDataPointer();
  for (unsigned int i = 0;i<pColorData->GetSize().area();i++) {
    *psDataIterator++ = (unsigned short)((*piSourceIterator)[0]*sUsedRange);
    *psDataIterator++ = (unsigned short)((*piSourceIterator)[1]*sUsedRange);
    *psDataIterator++ = (unsigned short)((*piSourceIterator)[2]*sUsedRange);
    *psDataIterator++ = (unsigned short)((*piSourceIterator)[3]*sUsedRange);
    piSourceIterator++;
  }
  delete pColorData;
}

void TransferFunction2D::GetFloatArray(float** pfData) {
  if (*pfData == NULL) *pfData = new float[4*m_iSize.area()];

  ColorData2D* pColorData = RenderTransferFunction();
  memcpy(*pfData, pColorData->GetDataPointer(), 4*sizeof(float)*m_iSize.area());
  delete pColorData;
}


ColorData2D* TransferFunction2D::RenderTransferFunction() {
  ColorData2D* pColorData = new ColorData2D(m_iSize);

  /// \todo (? undocumented)

  return pColorData;
}


void TFPolygon::Load(ifstream& file) {
  unsigned int iSize;
  file >> iSize;
  pPoints.resize(iSize);

  for(unsigned int i=0;i<pPoints.size();++i){
    for(unsigned int j=0;j<2;++j){
      file >> pPoints[i][j];
    }
  }

  file >> pGradientCoords[0][0] >> pGradientCoords[0][1];
  file >> pGradientCoords[1][0] >> pGradientCoords[1][1];

  file >> iSize;
  pGradientStops.resize(iSize);
  for(unsigned int i=0;i<pGradientStops.size();++i){
    file >> pGradientStops[i].first;
    for(unsigned int j=0;j<4;++j){
      file >> pGradientStops[i].second[j];
    }
  }

}

void TFPolygon::Save(ofstream& file) {
  file << pPoints.size() << endl;

  for(unsigned int i=0;i<pPoints.size();++i){
    for(unsigned int j=0;j<2;++j){
      file << pPoints[i][j] << " ";
    }
    file << endl;
  }

  file << pGradientCoords[0][0] << " " << pGradientCoords[0][1] << " ";
  file << pGradientCoords[1][0] << " " << pGradientCoords[1][1];
  file << endl;
  file << pGradientStops.size() << endl;

  for(unsigned int i=0;i<pGradientStops.size();++i){
    file << pGradientStops[i].first << "  ";
    for(unsigned int j=0;j<4;++j){
      file << pGradientStops[i].second[j] << " ";
    }
    file << endl;
  }  
}
