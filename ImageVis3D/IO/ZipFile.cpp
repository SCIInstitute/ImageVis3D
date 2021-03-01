#include "ZipFile.h"

#include <ctime>
#include <fstream>
#include <algorithm>

#include "IO/3rdParty/zip.h"

ZipFile::ZipFile()
  : m_zf(nullptr), m_writable(false), m_selectedFilename()
{

}

ZipFile::~ZipFile()
{
  close();
}

bool ZipFile::openZip(std::string const& filename)
{
  close();

  m_zf = zipOpen(filename.c_str(), APPEND_STATUS_CREATE);
  return (m_zf != nullptr);
}

bool ZipFile::selectFileInZip(std::string const& filename)
{
  if (m_zf == nullptr)
    return m_writable = false;

  if (m_writable) {
    // close previously opened file in zip
    int err = zipCloseFileInZip(m_zf);
    if (err != ZIP_OK)
      return m_writable = false;
  }

  std::time_t t = std::time(nullptr); // now
  std::tm * now = std::localtime(&t);

  zip_fileinfo fi;
  fi.tmz_date.tm_sec = now->tm_sec;
  fi.tmz_date.tm_min = now->tm_min;
  fi.tmz_date.tm_hour = now->tm_hour;
  fi.tmz_date.tm_mday = now->tm_mday;
  fi.tmz_date.tm_mon = now->tm_mon;
  fi.tmz_date.tm_year = now->tm_year;
  fi.dosDate = 0;
  fi.internal_fa = 0;
  fi.external_fa = 0;

  int err = zipOpenNewFileInZip(m_zf, filename.c_str(), &fi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

  if (err == ZIP_OK)
    m_selectedFilename = filename;

  return m_writable = (err == ZIP_OK);
}

bool ZipFile::copyFileToZip(std::string const& srcFilename, std::string const& dstFilename)
{
  if (!selectFileInZip(dstFilename.empty() ? srcFilename : dstFilename))
    return false;

  std::ifstream fs;
  fs.open(srcFilename.c_str(), std::fstream::in | std::fstream::binary);

  bool success = false;

  if (fs.is_open())
  {
    success = true;
    fs.seekg(0, fs.end);
    size_t fileSize = fs.tellg();
    fs.seekg(0, fs.beg);

    const size_t bufferSize = std::min(size_t(1024 * 1024 * 10), fileSize);
    char * buffer = new char[bufferSize];
    size_t bytesTotal = 0;

    while (fs.good())
    {
      fs.read(buffer, bufferSize);
      size_t readBytes = fs.gcount();
      if (!writeToZip(buffer, (uint32_t)readBytes))
      {
        success = false;
        break;
      }
      bytesTotal += readBytes;
    }

    fs.close();
    success = (fileSize == bytesTotal);

    // clean up
    if (buffer != nullptr) {
      delete[] buffer;
      buffer = nullptr;
    }
  }
  return success;
}

bool ZipFile::writeToZip(char const * const bytes, uint32_t length)
{
  if (m_zf == nullptr || m_writable == false)
    return false;

  int err = zipWriteInFileInZip(m_zf, bytes, length);
  return (err == ZIP_OK);
}

bool ZipFile::close()
{
  if (m_zf == nullptr)
    return false;

  int err = 0;
  
  if (m_writable) {
    err = zipCloseFileInZip(m_zf);
    m_writable = false;
    m_selectedFilename = "";
  }

  err = zipClose(m_zf, nullptr);
  m_zf = nullptr;

  return (err == ZIP_OK);
}
