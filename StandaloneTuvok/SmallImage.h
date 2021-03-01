#ifndef SMALLIMAGE_H
#define SMALLIMAGE_H

#include "StdTuvokDefines.h"
#include <cstdint>
#include <string>
#include "Basics/Vectors.h"

typedef VECTOR3<uint8_t> Color;

class SmallImage
{
public:
  SmallImage(unsigned int width, unsigned int height, unsigned int iComponentCount);
  SmallImage(const std::string& filename);
  virtual ~SmallImage(void);

  static bool PeekBMPHeader(const std::string& filename, UINTVECTOR2& size, unsigned int& iComponentCount);
	
  bool SaveToRAWFile(const std::string& filename) const;
  bool SaveToBMPFile(const std::string& filename) const;

  void SetPixel(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  void SetPixel(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);
  void SetPixel(unsigned int x, unsigned int y, uint8_t grey);
  void SetPixel(unsigned int x, unsigned int y, const Color& c);

  void GetPixel(unsigned int x, unsigned int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;
  void GetPixel(unsigned int x, unsigned int y, uint8_t& r, uint8_t& g, uint8_t& b) const;
  void GetPixel(unsigned int x, unsigned int y, uint8_t& grey) const;
  void GetPixel(unsigned int x, unsigned int y, Color& c) const;
  Color GetPixel(unsigned int x, unsigned int y) const;

  int ComponentCount() const {return m_iComponentCount;}
  int Height() const {return m_size.y;}
  int Width() const {return m_size.x;}
  int Area() const {return m_size.area();}
  const UINTVECTOR2& GetSize() const {return m_size;}

  void ForceComponentCount(unsigned int newCompCount, uint8_t padValue=255);
  void Resample(unsigned int newWidth, unsigned int newHeight, bool bKeepAspect=false);
  SmallImage* GeneratePreviewImage(unsigned int newWidth, unsigned int newHeight, bool bKeepAspect=false);
  
  const uint8_t* GetDataPtr() const { return m_pData;}
  uint8_t* GetDataPtrRW() { return m_pData;}

	
private:
  UINTVECTOR2    m_size;
  unsigned int   m_iComponentCount;
  uint8_t       *m_pData;
	
  static bool PeekBMPHeader(const std::string& filename, UINTVECTOR2& size, unsigned int& iComponentCount, bool& bUpsideDown, int& iOffsetToData);
	
  void InitData();
  bool LoadFromBMP(const std::string& filename);

  size_t OneDIndex(unsigned int x, unsigned int y) const { return size_t(m_iComponentCount*(x+y*m_size.x)); }
  void Resample(uint8_t* pTarget, unsigned int newWidth, unsigned int newHeight);
  void AdjustToAspect(unsigned int& newWidth, unsigned int& newHeight);
};

#endif // SMALLIMAGE_H
