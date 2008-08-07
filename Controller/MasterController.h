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
	\file		MasterController.h
	\author		Jens Krueger
				SCI Institute
				University of Utah
	\version	1.0
	\date		August 2008
*/


#pragma once

#ifndef MASTERCONTROLLER_H
#define MASTERCONTROLLER_H

#include <vector>
#include "../IO/TransferFunction1D.h"
#include "../IO/TransferFunction2D.h"

#include "../DebugOut/AbstrDebugOut.h"
#include "../DebugOut/ConsoleOut.h"

#include "../Renderer/GPUMemMan.h"
#include "../Renderer/AbstrRenderer.h"
#include "../Renderer/GPUSBVR.h"

enum VolumeRenderer {
	OPENGL_SBVR = 0
};

class MasterController {
	public:
		MasterController();
		virtual ~MasterController();

		AbstrRenderer* RequestNewVolumerenderer(VolumeRenderer eRendererType);
		void ReleaseVolumerenderer(AbstrRenderer* pVolumeRenderer);
		
		void SetDebugOut(AbstrDebugOut* debugOut);
		void RemoveDebugOut(AbstrDebugOut* debugOut);

		AbstrDebugOut* DebugOut() {return m_pDebugOut;}
		GPUMemMan*	   MemMan() {return m_pGPUMemMan;}
		
	private:
		GPUMemMan*     m_pGPUMemMan;
		AbstrDebugOut* m_pDebugOut;
		bool		   m_bStartDebugOut;	

		AbstrRendererList					m_vVolumeRenderer;
};

#endif // MASTERCONTROLLER_H
