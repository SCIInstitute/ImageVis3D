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
  \file    Culling.cpp
  \brief    Simple conservative frustum culling
  \author   Jens Krueger
            SCI Institute
            University of Utah
  \version  1.0
  \date    October 2008
*/

#include "Culling.h"

void Culling::SetParameters(const FLOATMATRIX4& mProjectionMatrix)
{
	m_mProjectionMatrix = mProjectionMatrix;
}

void Culling::Update(const FLOATMATRIX4& mWorldViewMatrix)
{
	FLOATMATRIX4 mWorldViewProjectionMatrix;

  mWorldViewProjectionMatrix =  m_mProjectionMatrix * mWorldViewMatrix; // TODO: check -> D3DXMatrixMultiply(&mWorldViewProjectionMatrix, mWorldViewMatrix, &m_mProjectionMatrix);

	// compute clip-planes in World Space

	// right clip-plane
	m_Planes[0].x = -mWorldViewProjectionMatrix.m11 + mWorldViewProjectionMatrix.m14;
	m_Planes[0].y = -mWorldViewProjectionMatrix.m21 + mWorldViewProjectionMatrix.m24;
	m_Planes[0].z = -mWorldViewProjectionMatrix.m31 + mWorldViewProjectionMatrix.m34;
	m_Planes[0].w = -mWorldViewProjectionMatrix.m41 + mWorldViewProjectionMatrix.m44;
	// left clip-plane
	m_Planes[1].x = mWorldViewProjectionMatrix.m11 + mWorldViewProjectionMatrix.m14;
	m_Planes[1].y = mWorldViewProjectionMatrix.m21 + mWorldViewProjectionMatrix.m24;
	m_Planes[1].z = mWorldViewProjectionMatrix.m31 + mWorldViewProjectionMatrix.m34;
	m_Planes[1].w = mWorldViewProjectionMatrix.m41 + mWorldViewProjectionMatrix.m44;
	// top clip-plane
	m_Planes[2].x = -mWorldViewProjectionMatrix.m12 + mWorldViewProjectionMatrix.m14;
	m_Planes[2].y = -mWorldViewProjectionMatrix.m22 + mWorldViewProjectionMatrix.m24;
	m_Planes[2].z = -mWorldViewProjectionMatrix.m32 + mWorldViewProjectionMatrix.m34;
	m_Planes[2].w = -mWorldViewProjectionMatrix.m42 + mWorldViewProjectionMatrix.m44;
	// bottom clip-plane
	m_Planes[3].x = mWorldViewProjectionMatrix.m12 + mWorldViewProjectionMatrix.m14;
	m_Planes[3].y = mWorldViewProjectionMatrix.m22 + mWorldViewProjectionMatrix.m24;
	m_Planes[3].z = mWorldViewProjectionMatrix.m32 + mWorldViewProjectionMatrix.m34;
	m_Planes[3].w = mWorldViewProjectionMatrix.m42 + mWorldViewProjectionMatrix.m44;
	// far clip-plane
	m_Planes[4].x = -mWorldViewProjectionMatrix.m13 + mWorldViewProjectionMatrix.m14;
	m_Planes[4].y = -mWorldViewProjectionMatrix.m23 + mWorldViewProjectionMatrix.m24;
	m_Planes[4].z = -mWorldViewProjectionMatrix.m33 + mWorldViewProjectionMatrix.m34;
	m_Planes[4].w = -mWorldViewProjectionMatrix.m43 + mWorldViewProjectionMatrix.m44;
	// near clip-plane
	m_Planes[5].x = mWorldViewProjectionMatrix.m13;
	m_Planes[5].y = mWorldViewProjectionMatrix.m23;
	m_Planes[5].z = mWorldViewProjectionMatrix.m33;
	m_Planes[5].w = mWorldViewProjectionMatrix.m43;

}

bool Culling::IsVisible(const FLOATVECTOR3& vCenter, const FLOATVECTOR3& vHalfExtent)  const
{
	for (unsigned int uiPlane = 0; uiPlane < 6; uiPlane++)
	{
		FLOATVECTOR4 plane = m_Planes[uiPlane];
		if (
			plane.x * vCenter.x + plane.y * vCenter.y + plane.z * vCenter.z + plane.w
			<= -(vHalfExtent.x * abs(plane.x) + vHalfExtent.y * abs(plane.y) + vHalfExtent.z * abs(plane.z))
			)
		{
			return false;
		}
	}
	return true;
}