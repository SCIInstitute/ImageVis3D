#ifndef IV3D_ZIP_FILE_H
#define IV3D_ZIP_FILE_H

#include <string>
#include <cstdint>

typedef void * zipFile;

class ZipFile {
public:
  ZipFile();
  ~ZipFile();
  bool openZip(std::string const& filename);
  bool selectFileInZip(std::string const& filename);
  bool copyFileToZip(std::string const& srcfilename, std::string const& dstFilename = "");
  bool writeToZip(char const * const bytes, uint32_t length);
  bool close();
  std::string const& getSelectedFilenameInZip() const { return m_selectedFilename; }
private:
  zipFile m_zf;
  bool m_writable;
  std::string m_selectedFilename;
};

#endif /* IV3D_ZIP_FILE_H */

/*
The MIT License

Copyright (c) 2011 Interactive Visualization and Data Analysis Group

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
