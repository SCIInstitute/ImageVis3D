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
	\file		GPUSBVR.h
	\author		Jens Krueger
				SCI Institute
				University of Utah
	\version	1.0
	\date		August 2008
*/


#pragma once

#ifndef GPUSBVR_H
#define GPUSBVR_H

#include "AbstrRenderer.h"
#include "../Renderer/GPUMemMan.h"
#include "../Renderer/GLTexture2D.h"

class GPUSBVR : public AbstrRenderer {
	public:
		GPUSBVR(MasterController* pMasterController);
		virtual ~GPUSBVR();

		void Initialize();
		void Paint();
		void Resize(int width, int height);
		void Cleanup();

		void SetRotation(int xRot) {m_xRot = xRot;}
		void SetCurrentView(int iCurrentView) {m_iCurrentView = iCurrentView;}
		int GetCurrentView() {return m_iCurrentView;}

	protected:
		GLTexture2D* m_IDTex[3];
		int m_iCurrentView;
		int m_xRot;
};

#endif // GPUSBVR_H
