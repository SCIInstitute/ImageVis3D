#include "SmallImage.h"
#include <fstream>


SmallImage::SmallImage(unsigned int width, unsigned int height, unsigned int iComponentCount) :
 m_size(width, height),
 m_iComponentCount(iComponentCount),
 m_pData(0)
{
  InitData();
}

SmallImage::SmallImage(const std::string& filename) :
  m_size(0,0),
  m_iComponentCount(0),
  m_pData(0)
{
  LoadFromBMP(filename);
}


SmallImage::~SmallImage(void)
{
  delete [] m_pData;
}

void SmallImage::InitData() {
  m_pData = new uint8_t[m_iComponentCount*m_size.area()];
  memset(m_pData,0,m_iComponentCount*m_size.area());
}

bool SmallImage::PeekBMPHeader(const std::string& filename, UINTVECTOR2& size, unsigned int& iComponentCount) {
	bool bUpsideDown;
	int iOffsetToData;
	return PeekBMPHeader(filename, size, iComponentCount, bUpsideDown, iOffsetToData);
}		

bool SmallImage::PeekBMPHeader(const std::string& filename, UINTVECTOR2& size, unsigned int& iComponentCount, bool& bUpsideDown, int& iOffsetToData) {    
	std::ifstream inStream(filename.c_str(), std::ofstream::binary);
	if (inStream.fail()) return false;
	
	// check "BM" magic bytes
	uint16_t bfType;
	inStream.read((char*)&bfType, 2);
	if (bfType != 19778) {
		inStream.close();
		return false;
	}
	
	// skip file size and reserved fields of bitmap file header
	inStream.seekg(8, std::ios_base::cur);
	// get the position of the actual bitmap data
	inStream.read((char*)&iOffsetToData, 4);
	inStream.seekg(4, std::ios_base::cur);    // skip size of bitmap info header    
	int w, h;
	inStream.read((char*)&w, 4);   // get the width of the bitmap    
	inStream.read((char*)&h, 4);   // get the hight of the bitmap    
	short int biPlanes;
	inStream.read((char*)&biPlanes, 2);   // get the number of planes
	if (biPlanes != 1) {
		inStream.close();
		return false;
	}
	// get the number of bits per pixel
	uint16_t biBitCount;
	inStream.read((char*)&biBitCount, 2);   // get the number of planes
	if (biBitCount != 24 && biBitCount != 32) {
		inStream.close();
		return false;
	}
	
	iComponentCount = biBitCount/8;
	size = UINTVECTOR2(w,abs(h));
	bUpsideDown = h < 0;
	
	inStream.close();
	
	return true;
}


bool SmallImage::LoadFromBMP(const std::string& filename) {    
  bool bUpsideDown;
  int iOffsetToData;
  if (!PeekBMPHeader(filename, m_size, m_iComponentCount, bUpsideDown, iOffsetToData)) {
    return false;
  }
	
  // seek to the actual data
  std::ifstream inStream(filename.c_str(), std::ofstream::binary);
  if (inStream.fail()) return false;
  inStream.seekg(iOffsetToData, std::ios_base::beg);

  delete m_pData;
  m_pData = new uint8_t[m_iComponentCount*m_size.area()];

  int rowPad= 4- (m_size.x*m_iComponentCount)%4;
  if (rowPad == 4) rowPad = 0;

  if (rowPad == 0 && bUpsideDown) 
    inStream.read((char*)m_pData, m_iComponentCount*m_size.area());
  else {
    for (unsigned int row = 0; row < m_size.y; row++ ) {
      int offset = (bUpsideDown ? row : ((m_size.y-1)-row)) * m_iComponentCount*m_size.x;
      inStream.read((char*)m_pData+offset, m_iComponentCount*m_size.x);
      if (rowPad > 0) inStream.seekg(rowPad, std::ios_base::cur);
    }
  }
  inStream.close();

  // swap red and blue (bgr[a] -> rgb[a])
  for (uint32_t i = 0; i < m_size.area()*m_iComponentCount; i += m_iComponentCount)
      std::swap(m_pData[i], m_pData[i+2]);

  return true;	
}


bool SmallImage::SaveToRAWFile(const std::string& filename) const {
  std::ofstream outStream(filename.c_str(), std::ofstream::binary);
  if (!outStream.is_open()) return false;
  outStream.write((char*)m_pData, m_iComponentCount*m_size.area());
  outStream.close();
  return true;
}


bool SmallImage::SaveToBMPFile(const std::string& filename) const  {
  std::ofstream outStream(filename.c_str(), std::ofstream::binary);
  if (!outStream.is_open()) return false;

  int h = m_size.y;
  int w = m_size.x;

	// write BMP-Header
  outStream.write((char*)"BM", 2); // all BMP-Files start with "BM"
	uint32_t header[3];
	int rowPad= 4-((w*8*m_iComponentCount)%32)/8;
  if (rowPad == 4) rowPad = 0;
	header[0] = 54+w*h*m_iComponentCount+rowPad*h;	// filesize = 54 (header) + sizeX * sizeY * numChannels
	header[1] = 0;						      // reserved = 0 (4 Bytes)
	header[2] = 54;						      // File offset to Raster Data
	outStream.write((char*)header, 4*3);
	// write BMP-Info-Header
	uint32_t infoHeader[10];
	infoHeader[0] = 40;	          // size of info header 
	infoHeader[1] = w;            // Bitmap Width
	infoHeader[2] = -h;           // Bitmap Height (negative to flip image)
	infoHeader[3] = 1+65536*8*m_iComponentCount;  
                                // first 2 bytes=Number of Planes (=1)
										            // next  2 bytes=BPP
	infoHeader[4] = 0;				  	// compression (0 = none)
	infoHeader[5] = 0;					  // compressed file size (0 if no compression)
	infoHeader[6] = 11810;				// horizontal resolution: Pixels/meter (11810 = 300 dpi)
	infoHeader[7] = 11810;				// vertical resolution: Pixels/meter (11810 = 300 dpi)
	infoHeader[8] = 0;					  // Number of actually used colors
	infoHeader[9] = 0;					  // Number of important colors  0 = all		
	outStream.write((char*)infoHeader, 4*10);

  // data in BMP is stored BGR, so convert RGB to BGR
  uint8_t* pData = new uint8_t [m_iComponentCount*m_size.area()];
  for (uint32_t i = 0; i < m_iComponentCount*m_size.area(); i+=m_iComponentCount) {
    pData[i]   = m_pData[i+2];
    pData[i+1] = m_pData[i+1];
    pData[i+2] = m_pData[i];
    if (m_iComponentCount==4) pData[i+3] = m_pData[i+3];
  }

  // write data (pad if necessary)
	if (rowPad==0) {
		outStream.write((char*)pData, m_iComponentCount*m_size.area());
	}
	else {
		uint8_t zeroes[9]={0,0,0,0,0,0,0,0,0};
		for (int i=0; i<h; i++) {
      outStream.write((char*)pData+m_iComponentCount*i*w, m_iComponentCount*w);
      outStream.write((char*)zeroes, rowPad);
		}
	}

  delete [] pData;
  outStream.close();
  return true;
}

void SmallImage::AdjustToAspect(unsigned int& newWidth, unsigned int& newHeight) {
  if (float(newWidth)/float(newHeight) > float(m_size.x)/float(m_size.y)) {
    newWidth = (unsigned int)(float(newHeight) * float(m_size.x)/float(m_size.y));
  } else {
    newHeight = (unsigned int)(float(newWidth) * float(m_size.y)/float(m_size.x));
  }
}

void SmallImage::Resample(uint8_t* pTarget, unsigned int newWidth, unsigned int newHeight) {

  // info: this code is very inneficient but it's easy to read
  //       and this is a SMALL-Image class after all :-)
  float deltaX = m_size.x / float(newWidth);
  float deltaY = m_size.y / float(newHeight);

  uint8_t* targetPtr = pTarget;
  for (unsigned int y = 0;y<newHeight;y++) {
    float floatY = deltaY*y;
    unsigned int cY = (unsigned int)(ceil(floatY));
    unsigned int fY = (unsigned int)(floor(floatY));
    for (unsigned int x = 0;x<newWidth;x++) {
      float floatX = deltaX*x;
      unsigned int cX = (unsigned int)(ceil(floatX));
      unsigned int fX = (unsigned int)(floor(floatX));

      size_t i0 = OneDIndex(fX,fY);
      size_t i1 = OneDIndex(cX,fY);
      size_t i2 = OneDIndex(fX,cY);
      size_t i3 = OneDIndex(cX,cY);

      float sX = floatX-fX;
      float sY = floatY-fY;

      for (size_t c = 0;c<m_iComponentCount;c++) {
        float val0 = float(m_pData[i0+c]);
        float val1 = float(m_pData[i1+c]);
        float val2 = float(m_pData[i2+c]);
        float val3 = float(m_pData[i3+c]);
        
        // billinear interpolation
        uint8_t val = uint8_t((val0*(1.0f-sX)+val1*sX)*(1.0f-sY) + (val2*(1.0f-sX)+val3*sX)*sY);
        (*targetPtr) = val;
        targetPtr++;
      }
    }
  }
}


SmallImage* SmallImage::GeneratePreviewImage(unsigned int newWidth, unsigned int newHeight, bool bKeepAspect) {
  if (bKeepAspect) AdjustToAspect(newWidth, newHeight);
  SmallImage* preview = new SmallImage(newWidth, newHeight, m_iComponentCount);
  Resample(preview->m_pData, newWidth, newHeight);
  return preview;
}

void SmallImage::ForceComponentCount(unsigned int newCompCount, uint8_t padValue) {
  if (newCompCount != 3 && newCompCount != 4) return; //unsupported component count

  if (newCompCount == m_iComponentCount)  return; // that was easy :-)
  uint8_t* pData = new uint8_t [newCompCount*m_size.area()];  

  uint8_t* targetPtr = pData;
  uint8_t* sourcePtr = m_pData;
  if (newCompCount < m_iComponentCount) {
    for (size_t i = 0;i<m_size.area();i++) {
      for (size_t c = 0;c<newCompCount;c++) {
        (*targetPtr) = (*sourcePtr);
        targetPtr++;
        sourcePtr++;
      }
      sourcePtr += m_iComponentCount-newCompCount;
    }
  } else {
    for (size_t i = 0;i<m_size.area();i++) {
      for (size_t c = 0;c<m_iComponentCount;c++) {
        (*targetPtr) = (*sourcePtr);
        targetPtr++;
        sourcePtr++;
      }
      for (size_t c = m_iComponentCount;c<newCompCount;c++) {
        (*targetPtr) = padValue;
        targetPtr++;
      }
    }
  }

  delete [] m_pData;
  m_pData = pData;
  m_iComponentCount = newCompCount;
}

void SmallImage::Resample(unsigned int newWidth, unsigned int newHeight, bool bKeepAspect) {
  if (newWidth == m_size.x && newHeight == m_size.y)  return; // that was easy :-)
  
  if (bKeepAspect) AdjustToAspect(newWidth, newHeight);

  uint8_t* pData = new uint8_t [m_iComponentCount*newWidth*newHeight];  
  Resample(pData, newWidth, newHeight);
  delete [] m_pData;
  m_pData = pData;
  m_size = UINTVECTOR2(newWidth, newHeight);
}

void SmallImage::SetPixel(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  size_t index = OneDIndex(x,y);
  m_pData[index+0] = r;
  m_pData[index+1] = g;
  m_pData[index+2] = b;
  if (m_iComponentCount == 4) m_pData[index+3] = a;
}


void SmallImage::SetPixel(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b) {
  SetPixel(x, y, r, g, b, 255);
}

void SmallImage::SetPixel(unsigned int x, unsigned int y, uint8_t grey) {
  SetPixel(x, y, grey, grey, grey);
}

void SmallImage::SetPixel(unsigned int x, unsigned int y, const Color& c) {
  SetPixel(x, y, c.x, c.y, c.z);
}

void SmallImage::GetPixel(unsigned int x, unsigned int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {    
  size_t index = OneDIndex(x,y);
  r = m_pData[index+0];
  g = m_pData[index+1];
  b = m_pData[index+2];
  a = (m_iComponentCount == 4) ? m_pData[index+2] : 255;
}

void SmallImage::GetPixel(unsigned int x, unsigned int y, uint8_t& r, uint8_t& g, uint8_t& b) const {    
  size_t index = OneDIndex(x,y);
  r = m_pData[index+0];
  g = m_pData[index+1];
  b = m_pData[index+2];
}

void SmallImage::GetPixel(unsigned int x, unsigned int y, uint8_t& grey) const {    
  uint8_t r, g, b;
  GetPixel(x,y, r,g,b);
  grey = (uint8_t)(((unsigned int)r+(unsigned int)g+(unsigned int)b)/3);
}

void SmallImage::GetPixel(unsigned int x, unsigned int y, Color& c) const {    
  GetPixel(x,y, c.x,c.y,c.z);
}

Color SmallImage::GetPixel(unsigned int x, unsigned int y) const {
  Color c;
  GetPixel(x,y, c.x,c.y,c.z);
  return c;
}

