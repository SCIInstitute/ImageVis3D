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
  \file    AbstrRenderer.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "AbstrRenderer.h"
#include "../Controller/MasterController.h"

using namespace std;

AbstrRenderer::AbstrRenderer(MasterController* pMasterController) :   
  m_pMasterController(pMasterController),
  m_bRedraw(true), 
  m_eRenderMode(RM_1DTRANS),
  m_eViewMode(VM_SINGLE),
  m_eBlendPrecision(BP_32BIT),
  m_bUseLigthing(true),
  m_pDataset(NULL),
  m_p1DTrans(NULL),
  m_p2DTrans(NULL),
  m_fSampleRateModifier(1.0f),
  m_fIsovalue(0.5f),
  m_vTextColor(1,1,1,1),
  m_fZoom(0.0f),
  m_bRenderGlobalBBox(false),
  m_bRenderLocalBBox(false),
  m_iIntraFrameCounter(0),
  m_iFrameCounter(0),
  m_iCheckCounter(0),
  m_iMaxLODIndex(0),
  m_iCurrentLODOffset(0),
  m_vWinSize(0,0)
{
  m_vBackgroundColors[0] = FLOATVECTOR3(0,0,0);
  m_vBackgroundColors[1] = FLOATVECTOR3(0,0,0);

  m_eWindowMode[0] = WM_3D;
  m_eWindowMode[1] = WM_CORONAL;
  m_eWindowMode[2] = WM_AXIAL;
  m_eWindowMode[3] = WM_SAGITTAL;

  m_fSliceIndex[0] = 0.5f;
  m_fSliceIndex[1] = 0.5f;
  m_fSliceIndex[2] = 0.5f;
  m_fSliceIndex[3] = 0.5f;
}


bool AbstrRenderer::LoadDataset(const string& strFilename) {
  if (m_pMasterController == NULL) return false;

  if (m_pMasterController->IOMan() == NULL) {
    m_pMasterController->DebugOut()->Error("AbstrRenderer::LoadDataset","Cannont load dataset because m_pMasterController->IOMan() == NULL");
    return false;
  }

  m_pDataset = m_pMasterController->IOMan()->LoadDataset(strFilename,this);

  if (m_pDataset == NULL) {
    m_pMasterController->DebugOut()->Error("AbstrRenderer::LoadDataset","IOMan call to load dataset failed");
    return false;
  }

  m_pMasterController->DebugOut()->Message("AbstrRenderer::LoadDataset","Load successful, initializing renderer!");


  std::vector<UINT64> vSmallestLOD = m_pDataset->GetInfo()->GetLODLevelCountND();
  for (size_t i = 0;i<vSmallestLOD.size();i++) vSmallestLOD[i] -= 1;

  UINT64 iMaxSmallestLOD = 0;
  for (size_t i = 0;i<vSmallestLOD.size();i++) if (iMaxSmallestLOD < vSmallestLOD[i]) iMaxSmallestLOD = vSmallestLOD[i];
  
  m_iMaxLODIndex = iMaxSmallestLOD;

  return true;
}

AbstrRenderer::~AbstrRenderer() {
  m_pMasterController->MemMan()->FreeDataset(m_pDataset, this);
  m_pMasterController->MemMan()->Free1DTrans(m_p1DTrans, this);
  m_pMasterController->MemMan()->Free2DTrans(m_p2DTrans, this);

}

void AbstrRenderer::SetRendermode(ERenderMode eRenderMode) 
{
  if (m_eRenderMode != eRenderMode) {
    m_eRenderMode = eRenderMode; 
    ScheduleCompleteRedraw();
  }  
}

void AbstrRenderer::SetViewmode(EViewMode eViewMode)
{
  if (m_eViewMode != eViewMode) {
    m_eViewMode = eViewMode; 
    ScheduleCompleteRedraw();
  }  
}


void AbstrRenderer::SetWindowmode(unsigned int iWindowIndex, EWindowMode eWindowMode)
{
  if (m_eWindowMode[iWindowIndex] != eWindowMode) {
    m_eWindowMode[iWindowIndex] = eWindowMode; 
    ScheduleCompleteRedraw();
    /// \todo only redraw the windows dependent on this change
  }  
}

void AbstrRenderer::SetUseLigthing(bool bUseLigthing) {
  if (m_bUseLigthing != bUseLigthing) {
    m_bUseLigthing = bUseLigthing; 
    ScheduleCompleteRedraw();
    /// \todo only redraw the windows dependent on this change
  }
}

void AbstrRenderer::SetBlendPrecision(EBlendPrecision eBlendPrecision) {
  if (m_eBlendPrecision != eBlendPrecision) {
    m_eBlendPrecision = eBlendPrecision;
    ScheduleCompleteRedraw();
  }
}


void AbstrRenderer::Changed1DTrans() {
  if (m_eRenderMode != RM_1DTRANS) {
    m_pMasterController->DebugOut()->Message("AbstrRenderer::Changed1DTrans","not using the 1D transferfunction at the moment, ignoring message");
  } else {
    m_pMasterController->DebugOut()->Message("AbstrRenderer::Changed1DTrans","complete redraw scheduled");
    ScheduleCompleteRedraw();
  }
}

void AbstrRenderer::Changed2DTrans() {
  if (m_eRenderMode != RM_2DTRANS) {
    m_pMasterController->DebugOut()->Message("AbstrRenderer::Changed2DTrans","not using the 2D transferfunction at the moment, ignoring message");
  } else {
    m_pMasterController->DebugOut()->Message("AbstrRenderer::Changed2DTrans","complete redraw scheduled");
    ScheduleCompleteRedraw();
  }
}


void AbstrRenderer::SetSampleRateModifier(float fSampleRateModifier) {
  if(m_fSampleRateModifier != fSampleRateModifier) {
    m_fSampleRateModifier = fSampleRateModifier;
    ScheduleCompleteRedraw();
  }
}

void AbstrRenderer::SetIsoValue(float fIsovalue) {
  if(m_fIsovalue != fIsovalue) {
    m_fIsovalue = fIsovalue;
    ScheduleCompleteRedraw();
  }
}

bool AbstrRenderer::CheckForRedraw() {
  if (m_iCurrentLODOffset > 0) {
    if (m_iCheckCounter == 0)  {
      m_bRedraw = true;
      m_pMasterController->DebugOut()->Message("AbstrRenderer::CheckForRedraw","Scheduled redraw as LOD is %i > 0", m_iCurrentLODOffset);
    } else m_iCheckCounter--;
  }
  return m_bRedraw;
}

void AbstrRenderer::Resize(const UINTVECTOR2& vWinSize) {
  m_vWinSize = vWinSize;
  m_ArcBall.SetWindowSize(vWinSize.x, vWinSize.y);
  ScheduleCompleteRedraw();
}


void AbstrRenderer::Click(UINTVECTOR2 vPosition) {
  m_ArcBall.Click(vPosition);
}

void AbstrRenderer::Release(UINTVECTOR2) {
  m_RestRot = m_Rot;
}

void AbstrRenderer::Drag(UINTVECTOR2 vPosition) {
  m_Rot = m_RestRot * m_ArcBall.Drag(vPosition).ComputeRotation();
  ScheduleCompleteRedraw();
}

void AbstrRenderer::Zoom(int iZoom) {
  m_fZoom += float(iZoom)/1000.0f;
  ScheduleCompleteRedraw();
}

void AbstrRenderer::SetGlobalBBox(bool bRenderBBox) {
  m_bRenderGlobalBBox = bRenderBBox;
  ScheduleCompleteRedraw();
}

void AbstrRenderer::SetLocalBBox(bool bRenderBBox) {
  m_bRenderLocalBBox = bRenderBBox;
  ScheduleCompleteRedraw();
}

void AbstrRenderer::ScheduleCompleteRedraw() {
  m_bRedraw = true;
  m_iCurrentLODOffset = m_iMaxLODIndex+1;
  m_iCheckCounter = 10;
}