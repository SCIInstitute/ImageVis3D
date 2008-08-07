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
	\file		AbstrRenderer.h
	\author		Jens Krueger
				SCI Institute
				University of Utah
	\version	1.0
	\date		August 2008
*/


#pragma once

#ifndef ABSTRRENDERER_H
#define ABSTRRENDERER_H

#include <string>

#include "../IO/VolumeDataset.h"
#include "../IO/TransferFunction1D.h"
#include "../IO/TransferFunction2D.h"
#include "../Renderer/GLTexture1D.h"
#include "../Renderer/GLTexture2D.h"

class MasterController;

enum ERenderMode {
	RM_1DTRANS = 0,
	RM_2DTRANS,
//	RM_ISOSURFACE,
	RM_INVALID
};

class AbstrRenderer {
	public:
		virtual ~AbstrRenderer();
		bool LoadDataset(const std::string& strFilename);

		VolumeDataset*		GetDataSet() {return m_pDataset;}
		TransferFunction1D* Get1DTrans() {return m_p1DTrans;}
		TransferFunction2D* Get2DTrans() {return m_p2DTrans;}
		void Set1DTrans(TransferFunction1D* p1DTrans) {m_p1DTrans = p1DTrans;}
		void Set2DTrans(TransferFunction2D* p2DTrans) {m_p2DTrans = p2DTrans;}
		void SetRendermode(ERenderMode eRenderMode) {m_eRenderMode = eRenderMode;}

	protected:
		MasterController*	m_pMasterController;
		VolumeDataset*		m_pDataset;
		TransferFunction1D* m_p1DTrans;
		GLTexture1D*		m_p1DTransTex;
		TransferFunction2D* m_p2DTrans;
		GLTexture2D*		m_p2DTransTex;
		ERenderMode			m_eRenderMode;
};

#endif // ABSTRRENDERER_H
