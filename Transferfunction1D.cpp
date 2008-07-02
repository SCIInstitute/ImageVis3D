#include "Transferfunction1D.h"

#include <fstream>

using namespace std;

Transferfunction1D::Transferfunction1D(unsigned int iSize)
{
	Resize(iSize);
}

Transferfunction1D::~Transferfunction1D(void)
{
}


void Transferfunction1D::Resize(unsigned int iSize) {
	pColorData.resize(iSize);
}

bool Transferfunction1D::Load(const std::string& filename) {
	ifstream file(filename.c_str());

	if (!file.is_open()) return false;

	unsigned int iSize;
	file >> iSize;
	pColorData.resize(iSize);

	for(unsigned int i=0;i<pColorData.size();++i){
		for(unsigned int j=0;j<4;++j){
			file >> pColorData[i][j];
		}
	}

	return true;
}

bool Transferfunction1D::Save(const std::string& filename) {
	ofstream file(filename.c_str());

	if (!file.is_open()) return false;

	file << pColorData.size() << " ";

	for(unsigned int i=0;i<pColorData.size();++i){
		for(unsigned int j=0;j<4;++j){
			file << pColorData[i][j] << " ";
		}
		file << endl;
	}

	return true;
}

void Transferfunction1D::GetByteArray(unsigned char** pcData, unsigned char cUsedRange) {
	if (*pcData == NULL) *pcData = new unsigned char[pColorData.size()];

	unsigned char *pcDataIterator = *pcData;
	for (unsigned int i = 0;i<pColorData.size();i++) {
		*pcDataIterator++ = (unsigned char)(pColorData[i].r*cUsedRange);
		*pcDataIterator++ = (unsigned char)(pColorData[i].g*cUsedRange);
		*pcDataIterator++ = (unsigned char)(pColorData[i].b*cUsedRange);
		*pcDataIterator++ = (unsigned char)(pColorData[i].a*cUsedRange);
	}
}

void Transferfunction1D::GetShortArray(unsigned short** psData, unsigned short sUsedRange) {
	if (*psData == NULL) *psData = new unsigned short[pColorData.size()];

	unsigned short *psDataIterator = *psData;
	for (unsigned int i = 0;i<pColorData.size();i++) {
		*psDataIterator++ = (unsigned short)(pColorData[i].r*sUsedRange);
		*psDataIterator++ = (unsigned short)(pColorData[i].g*sUsedRange);
		*psDataIterator++ = (unsigned short)(pColorData[i].b*sUsedRange);
		*psDataIterator++ = (unsigned short)(pColorData[i].a*sUsedRange);
	}
}

void Transferfunction1D::GetFloatArray(float** pfData) {
	if (*pfData == NULL) *pfData = new float[pColorData.size()];
	memcpy(*pfData, &pfData[0], sizeof(float)*pColorData.size());
}
