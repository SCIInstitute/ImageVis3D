#pragma once

#ifndef TRANSFERFUNCTION1D
#define TRANSFERFUNCTION1D

#include <string>
#include <vector>

template <class T=float> class TFColor {
public:
	T r, g, b, a;	

	operator T*(void) {return &r;}
	const T *operator *(void) const {return &r;}
	T *operator *(void) {return &r;}
};

class Transferfunction1D
{
public:
	Transferfunction1D(unsigned int iSize = 0);
	~Transferfunction1D(void);
	
	void Resize(unsigned int iSize);

	bool Load(const std::string& filename);
	bool Save(const std::string& filename);

	void GetByteArray(unsigned char** pcData, unsigned char cUsedRange=255);
	void GetShortArray(unsigned short** psData, unsigned short sUsedRange=4095);
	void GetFloatArray(float** pfData);

	std::vector< TFColor<float> > pColorData;
};

#endif // TRANSFERFUNCTION1D
