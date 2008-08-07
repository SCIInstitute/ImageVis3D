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
	\file		TransferFunction2D.cpp
	\author		Jens Krueger
				SCI Institute
				University of Utah
	\version	1.0
	\date		July 2008
*/

#include "TransferFunction2D.h"

using namespace std;

TransferFunction2D::TransferFunction2D()
{
	Resize(VECTOR2<size_t>(0,0));
}

TransferFunction2D::TransferFunction2D(const VECTOR2<size_t>& iSize)
{
	Resize(iSize);
}

TransferFunction2D::~TransferFunction2D(void)
{
}

void TransferFunction2D::Resize(const VECTOR2<size_t>& iSize) {
	pColorData.Resize(iSize);
}

bool TransferFunction2D::Load(const std::string& filename) {
	ifstream file(filename.c_str());

	if (!file.is_open()) return false;

	// load gridsize
	VECTOR2<size_t> iSize;
	file >> iSize.x;
	file >> iSize.y;
	pColorData.Resize(iSize);

	// load swatch count
	unsigned int iSwatchCount;
	file >> iSwatchCount;
	m_Swatches.resize(iSwatchCount);

	// load Swatches
	for (unsigned int i = 0;i<m_Swatches.size();i++) m_Swatches[i].Load(file);


	return true;
}

bool TransferFunction2D::Save(const std::string& filename) {
	ofstream file(filename.c_str());

	if (!file.is_open()) return false;

	// save gridsize
	file << pColorData.GetSize().x << " " << pColorData.GetSize().y << " ";

	// save swatch count
	file << m_Swatches.size() << " ";

	// save Swatches
	for (unsigned int i = 0;i<m_Swatches.size();i++) m_Swatches[i].Save(file);

	return true;
}

void TransferFunction2D::GetByteArray(unsigned char** pcData, unsigned char cUsedRange) {
	if (*pcData == NULL) *pcData = new unsigned char[pColorData.GetSize().area()];

	unsigned char *pcDataIterator = *pcData;
	FLOATVECTOR4  *piSourceIterator = pColorData.GetDataPointer();
	for (unsigned int i = 0;i<pColorData.GetSize().area();i++) {
		*pcDataIterator++ = (unsigned char)((*piSourceIterator)[0]*cUsedRange);
		*pcDataIterator++ = (unsigned char)((*piSourceIterator)[1]*cUsedRange);
		*pcDataIterator++ = (unsigned char)((*piSourceIterator)[2]*cUsedRange);
		*pcDataIterator++ = (unsigned char)((*piSourceIterator)[3]*cUsedRange);
		piSourceIterator++;
	}
}

void TransferFunction2D::GetShortArray(unsigned short** psData, unsigned short sUsedRange) {
	if (*psData == NULL) *psData = new unsigned short[pColorData.GetSize().area()];

	unsigned short *psDataIterator = *psData;
	FLOATVECTOR4  *piSourceIterator = pColorData.GetDataPointer();
	for (unsigned int i = 0;i<pColorData.GetSize().area();i++) {
		*psDataIterator++ = (unsigned short)((*piSourceIterator)[0]*sUsedRange);
		*psDataIterator++ = (unsigned short)((*piSourceIterator)[1]*sUsedRange);
		*psDataIterator++ = (unsigned short)((*piSourceIterator)[2]*sUsedRange);
		*psDataIterator++ = (unsigned short)((*piSourceIterator)[3]*sUsedRange);
		piSourceIterator++;
	}
}

void TransferFunction2D::GetFloatArray(float** pfData) {
	if (*pfData == NULL) *pfData = new float[4*pColorData.GetSize().area()];
	memcpy(*pfData, pColorData.GetDataPointer(), 4*sizeof(float)*pColorData.GetSize().area());
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
	file << pGradientStops.size() << " ";

	for(unsigned int i=0;i<pGradientStops.size();++i){
		file << pGradientStops[i].first << endl;
		for(unsigned int j=0;j<4;++j){
			file << pGradientStops[i].second[j] << " ";
		}
		file << endl;
	}

}
