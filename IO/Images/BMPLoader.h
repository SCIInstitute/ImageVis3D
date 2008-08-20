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
  \file    BMPLoader.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/

#ifndef BMPLOADER_H
#define BMPLOADER_H

#include <string>

#ifdef WIN32
  #pragma warning(disable:4996)
#endif

typedef struct {
    int width;
    int height;
    unsigned char *data;
} TextureImage;

class BMPLoader {
  public:

    static bool Load(const std::string& strFilename, TextureImage *texture) {
      FILE *file;
      unsigned short int bfType;
      int bfOffBits;
      short int biPlanes;
      short int biBitCount;
      int biSizeImage;
      int i;
      unsigned char temp;
      // make sure file exists.
      if (!(file = fopen(strFilename.c_str(), "rb"))) return false; //File not found
      // make sure file can be read
      if(!fread(&bfType, sizeof(short int), 1, file)) return false; // File could not be read
      // check if file is a bitmap
      if (bfType != 19778) return false; // Not a bitmap
      // get the file size
      // skip file size and reserved fields of bitmap file header
      fseek(file, 8, SEEK_CUR);
      // get the position of the actual bitmap data
      if (!fread(&bfOffBits, sizeof(int), 1, file)) return false; // File could not be read
      fseek(file, 4, SEEK_CUR);                       // skip size of bitmap info header    
      fread(&texture->width, sizeof(int), 1, file);   // get the width of the bitmap    
      fread(&texture->height, sizeof(int), 1, file);  // get the height of the bitmap  
      fread(&biPlanes, sizeof(short int), 1, file);   // get the number of planes
      if (biPlanes != 1)    
        return false; // Number of bitplanes was not 1
      // get the number of bits per pixel
      if (!fread(&biBitCount, sizeof(short int), 1, file))            
        return false; // Error Reading file
      if (biBitCount != 24)    
        return false; // File not 24Bpp
      // calculate the size of the image in bytes
      biSizeImage = texture->width * texture->height * 3;    
      texture->data = new unsigned char[biSizeImage];
      // seek to the actual data
      fseek(file, bfOffBits, SEEK_SET);
      if (!fread(texture->data, biSizeImage, 1, file))    
        return false; // Error loading file
      fclose(file);
      // swap red and blue (bgr -> rgb)
      for (i = 0; i < biSizeImage; i += 3) {
        temp = texture->data[i];
        texture->data[i] = texture->data[i + 2];
        texture->data[i + 2] = temp;
      }
      return true;
    }
};

#endif // BMPLOADER_H
