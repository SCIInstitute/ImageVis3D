#include "LargeRAWFile.h"

using namespace std;

#define BLOCK_COPY_SIZE (UINT64(128*1024*1024))

LargeRAWFile::LargeRAWFile(string strFilename) :
  m_strFilename(strFilename),
  m_bIsOpen(false),
  m_bWritable(false)
{
}

LargeRAWFile::LargeRAWFile(wstring wstrFilename) :
  m_bIsOpen(false),
  m_bWritable(false)
{
  string strFilename(wstrFilename.begin(), wstrFilename.end());
  m_strFilename = strFilename;
}

LargeRAWFile::LargeRAWFile(LargeRAWFile &other) :
  m_strFilename(other.m_strFilename+"~"),
  m_bIsOpen(other.m_bIsOpen)
{
  if (m_bIsOpen) {
    UINT64 iDataSize = other.GetCurrentSize();
    Create(iDataSize);

    other.SeekStart();

    unsigned char* pData = new unsigned char[size_t(min(iDataSize, BLOCK_COPY_SIZE))];
    for (UINT64 i = 0;i<iDataSize;i+=BLOCK_COPY_SIZE) {
		  UINT64 iCopySize = min(BLOCK_COPY_SIZE, iDataSize-i);

      other.ReadRAW(pData, iCopySize);
      WriteRAW(pData, iCopySize);
    }
    delete [] pData;    
  }
}

bool LargeRAWFile::Open(bool bReadWrite) {
  #ifdef _WIN32
    int iOpenMode = _O_BINARY | _O_NOINHERIT | _O_RANDOM;
    if ( bReadWrite )	iOpenMode |= _O_RDWR; else iOpenMode |= _O_RDONLY; 
    
    errno_t error =_sopen_s(&m_StreamFile,m_strFilename.c_str(), iOpenMode,	_SH_DENYNO,	_S_IREAD | _S_IWRITE);
	  m_bIsOpen = !(error || m_StreamFile<0);
  #else
    m_StreamFile = fopen(m_strFilename.c_str(), (bReadWrite) ? "w+b" : "rb");
    m_bIsOpen = m_StreamFile != NULL;
  #endif

  m_bWritable = (m_bIsOpen) ? bReadWrite : false;

  return m_bIsOpen;
}

bool LargeRAWFile::Create(UINT64 iInitialSize) {
#ifdef _WIN32
  int iOpenMode = _O_BINARY | _O_NOINHERIT | _O_RANDOM | _O_CREAT | _O_RDWR | _O_TRUNC;
  
  errno_t error =_sopen_s(&m_StreamFile,m_strFilename.c_str(), iOpenMode,	SH_DENYNO,	_S_IREAD | _S_IWRITE);
	m_bIsOpen = !(error || m_StreamFile<0);
#else
  m_StreamFile = fopen(m_strFilename.c_str(), "w+b");
  m_bIsOpen = m_StreamFile != NULL;
#endif

  if (m_bIsOpen) {
    SeekPos(iInitialSize);
    SeekStart();
  }

  return m_bIsOpen;
}


void LargeRAWFile::Close() {
  if (m_bIsOpen) {
    #ifdef _WIN32
	    _close(m_StreamFile);
    #else
      fclose(m_StreamFile);
    #endif
    m_bIsOpen =false;
  }
}

UINT64 LargeRAWFile::GetCurrentSize() {
  UINT64 iPrevPos = GetPos();

  SeekStart();
	UINT64 ulStart = GetPos();
  UINT64 ulEnd   = SeekEnd();
  UINT64 ulFileLength = ulEnd - ulStart;
  
  SeekPos(iPrevPos);

  return ulFileLength;
}

void LargeRAWFile::SeekStart(){
  #ifdef _WIN32
   	_lseeki64(m_StreamFile,0,SEEK_SET);
  #else
    fseeko(m_StreamFile, 0, SEEK_SET);
  #endif
}

UINT64 LargeRAWFile::SeekEnd(){
  #ifdef _WIN32
   	return _lseeki64(m_StreamFile,0,SEEK_END);
  #else
    if(fseeko(m_StreamFile, 0, SEEK_END)==0)
      return ftello(m_StreamFile);//get current position=file size!
    else
      return 0;
  #endif
}

UINT64 LargeRAWFile::GetPos(){
  #ifdef _WIN32
   	return _telli64(m_StreamFile);
  #else
      return ftello(m_StreamFile);//get current position=file size!
  #endif
}

void LargeRAWFile::SeekPos(UINT64 iPos){
  #ifdef _WIN32
  	_lseeki64(m_StreamFile,iPos,SEEK_SET);
  #else
    fseeko(m_StreamFile, off_t(iPos), SEEK_SET);
  #endif
}

size_t LargeRAWFile::ReadRAW(unsigned char* pData, UINT64 iCount){
  #ifdef _WIN32
    return _read(m_StreamFile,pData,(unsigned int)(iCount));
  #else
    return fread(pData,1,iCount,m_StreamFile);
  #endif
}

size_t LargeRAWFile::WriteRAW(const unsigned char* pData, UINT64 iCount){
  #ifdef _WIN32
    return _write(m_StreamFile,pData,(unsigned int)(iCount));
  #else
    return fwrite(pData,1,iCount,m_StreamFile);
  #endif
}

void LargeRAWFile::Delete() {
  if (m_bIsOpen) Close();
  remove(m_strFilename.c_str());
}
