/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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

/// \author James Hughes
/// \date   December 2012

#ifndef BATCHRENDERER_NSCONTEXT_H
#define BATCHRENDERER_NSCONTEXT_H

#include <memory>
#include "BatchContext.h"

namespace tuvok
{

struct NSContextInfo;

/// Uses Objective-C to obtain a context.
class NSContext : public BatchContext
{
public:
  NSContext(uint32_t w, uint32_t h, uint8_t colorBits,
            uint8_t depthBits, uint8_t stencilBits,
            bool doubleBuffer, bool visible);
  virtual ~NSContext();

  bool isValid() const;
  bool makeCurrent();
  bool swapBuffers();

private:

  std::shared_ptr<NSContextInfo>  mCI;
};

} // namespace tuvok

#endif 
