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
  \file    GLRenderer.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "GLRenderer.h"
#include <Controller/MasterController.h>
#include <Basics/SysTools.h>

using namespace std;

GLRenderer::GLRenderer(MasterController* pMasterController) : 
  AbstrRenderer(pMasterController),
  m_p1DTransTex(NULL),
  m_p2DTransTex(NULL),
  m_p1DData(NULL),
  m_p2DData(NULL)
{
}

GLRenderer::~GLRenderer() {
}

bool GLRenderer::Initialize() {
  string strPotential1DTransName = SysTools::ChangeExt(m_pDataset->Filename(), "1dt");
  string strPotential2DTransName = SysTools::ChangeExt(m_pDataset->Filename(), "2dt");
  
  if (SysTools::FileExists(strPotential1DTransName)) 
    m_pMasterController->MemMan()->Get1DTransFromFile(strPotential1DTransName, this, &m_p1DTrans, &m_p1DTransTex);
  else
    m_pMasterController->MemMan()->GetEmpty1DTrans(m_pDataset->Get1DHistogram()->GetFilledSize(), this, &m_p1DTrans, &m_p1DTransTex);

  if (SysTools::FileExists(strPotential2DTransName)) 
    m_pMasterController->MemMan()->Get2DTransFromFile(strPotential2DTransName, this, &m_p2DTrans, &m_p2DTransTex);
  else
    m_pMasterController->MemMan()->GetEmpty2DTrans(m_pDataset->Get2DHistogram()->GetFilledSize(), this, &m_p2DTrans, &m_p2DTransTex);

  return true;
}

void GLRenderer::Changed1DTrans() {
  m_p1DTrans->GetByteArray(&m_p1DData);
  m_p1DTransTex->SetData(m_p1DData);

  AbstrRenderer::Changed1DTrans();
}

void GLRenderer::Changed2DTrans() {
  m_p2DTrans->GetByteArray(&m_p2DData);
  m_p2DTransTex->SetData(m_p2DData);

  AbstrRenderer::Changed2DTrans();
}

bool GLRenderer::SetBackgroundColors(FLOATVECTOR3 vColors[2]) {
  if (AbstrRenderer::SetBackgroundColors(vColors)) {
    glClearColor(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,0);
    return true;
  } else return false;
}

