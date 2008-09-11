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
  \file    SBVRGeogen.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    September 2008
*/

#pragma once

#include <vector>
#include <Basics/vectors.h>

class POS3TEX3_VERTEX
{
public:
	POS3TEX3_VERTEX() : m_vPos() , m_vTex() {}
	POS3TEX3_VERTEX(const FLOATVECTOR3 &vPos, const FLOATVECTOR3 &vTex) : m_vPos(vPos) , m_vTex(vTex) {}
	POS3TEX3_VERTEX(const FLOATVECTOR4 &vPos, const FLOATVECTOR3 &vUnTransPos) : m_vPos(vPos.xyz())  {
		m_vTex = vUnTransPos + 0.5f;
	}
	POS3TEX3_VERTEX(const FLOATVECTOR3 &vPos) : m_vPos(vPos)  {
		m_vTex = m_vPos + 0.5f;
	}

	FLOATVECTOR3 m_vPos;
	FLOATVECTOR3 m_vTex;
};

class SBVRGeogen
{
public:
	SBVRGeogen(void);
	~SBVRGeogen(void);

	virtual void SetTransformation(const FLOATMATRIX4& matTransform);
	virtual void SetVolumeData(	const FLOATVECTOR3& vAspect, const UINTVECTOR3& vSize) {m_vAspect = vAspect; m_vSize = vSize;  InitBBOX(); }
	void ComputeGeometry();
	uint ComputeLayerGeometry(float fDepth, POS3TEX3_VERTEX pfLayerPoints[6]);

	std::vector<POS3TEX3_VERTEX> m_vSliceTriangles;

protected:

	FLOATMATRIX4		  m_matTransform;
	float				      m_fMinZ;
	POS3TEX3_VERTEX		m_pfBBOXVertex[8];
	FLOATVECTOR3		  m_pfBBOXStaticVertex[8];
	FLOATVECTOR3		  m_vAspect;
	UINTVECTOR3			  m_vSize;

	void InitBBOX();
	bool EpsilonEqual(float a, float b);
	bool ComputeLayerGeometry(float fDepth);
	void ComputeIntersection(float z, uint indexA, uint indexB, POS3TEX3_VERTEX& vHit, uint &count);
	bool CheckOdering(FLOATVECTOR3& a, FLOATVECTOR3& b, FLOATVECTOR3& c);
	void Swap(POS3TEX3_VERTEX& a, POS3TEX3_VERTEX& b);
	void SortPoints(POS3TEX3_VERTEX fArray[6], uint iCount);
	int FindMinPoint(POS3TEX3_VERTEX fArray[6], uint iCount);
	void Triangulate(POS3TEX3_VERTEX fArray[6], uint iCount);
};
