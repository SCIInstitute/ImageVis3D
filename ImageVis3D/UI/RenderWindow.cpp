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


//!    File   : RenderWindow.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "RenderWindow.h"
#include "ImageVis3D.h"

#include <QtGui/QtGui>
#include <assert.h>
#include <sstream>
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"

using namespace std;

std::string RenderWindow::ms_gpuVendorString = "";
UINT32 RenderWindow::ms_iMax3DTexDims = 0;

RenderWindow::RenderWindow(MasterController& masterController,
                           MasterController::EVolumeRendererType eType,
                           const QString& dataset,
                           unsigned int iCounter,
                           QWidget* parent,
                           const UINTVECTOR2& vMinSize,
                           const UINTVECTOR2& vDefaultSize) :
  m_strDataset(dataset),
  m_strID(""),
  m_Renderer(NULL),
  m_MasterController(masterController),
  m_bRenderSubsysOK(true),   // be optimistic :-)
  m_vWinDim(0,0),
  m_vMinSize(vMinSize),
  m_vDefaultSize(vDefaultSize),
  m_eRendererType(eType),
  m_MainWindow((MainWindow*)parent),
  m_iTimeSliceMSecsActive(500),
  m_iTimeSliceMSecsInActive(100),
  m_viRightClickPos(0,0),
  m_viMousePos(0,0),
  m_bAbsoluteViewLock(true),
  m_bCaptureMode(false)
{  
  m_strID = "[%1] %2";
  m_strID = m_strID.arg(iCounter).arg(dataset);
}

RenderWindow::~RenderWindow()
{
  Cleanup();
}


void RenderWindow::SetAvoidCompositing(bool bAvoidCompositing) {
  m_Renderer->SetAvoidSeperateCompositing(bAvoidCompositing);
}

bool RenderWindow::GetAvoidCompositing() const {
  return m_Renderer->GetAvoidSeperateCompositing();
}

void RenderWindow::ToggleHQCaptureMode() {
  m_bCaptureMode = !m_bCaptureMode;
  if (m_bCaptureMode) {
    m_mCaptureStartRotation = m_mAccumulatedRotation;
  } else {
    m_mAccumulatedRotation = m_mCaptureStartRotation;
  }
  m_Renderer->SetCaptureMode(m_bCaptureMode);
}

void RenderWindow::SetCaptureRotationAngle(float fAngle) {
  FLOATMATRIX4 matRot;
  matRot.RotationY(3.141592653589793238462643383*double(fAngle)/180.0);
  matRot = m_mCaptureStartRotation * matRot;
  SetRotation(matRot, matRot);
  PaintRenderer();
}

void RenderWindow::MousePressEvent(QMouseEvent *event)
{
  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    if (event->button() == Qt::RightButton) m_viRightClickPos = INTVECTOR2(event->pos().x(), event->pos().y());
    if (event->button() == Qt::LeftButton)  m_ArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
  }
}

void RenderWindow::MouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) FinalizeRotation(true);
}

void RenderWindow::MouseMoveEvent(QMouseEvent *event)
{
  m_viMousePos = INTVECTOR2(event->pos().x(), event->pos().y());
  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    bool bPerformUpdate = false;

    if (m_Renderer->GetRendermode() == AbstrRenderer::RM_ISOSURFACE &&
        m_Renderer->GetCV() &&
        event->modifiers() & Qt::ShiftModifier) {
      SetCVFocusPos(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    }

    if (event->buttons() & Qt::LeftButton) {
      SetRotationDelta(m_ArcBall.Drag(UINTVECTOR2(event->pos().x(), event->pos().y())).ComputeRotation(),true);
      bPerformUpdate = true;
    }

    if (event->buttons() & Qt::RightButton) {
      INTVECTOR2 viPosDelta = m_viMousePos - m_viRightClickPos;
      m_viRightClickPos = m_viMousePos;
      SetTranslationDelta(FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x, float(viPosDelta.y*2) / m_vWinDim.y,0),true);
      bPerformUpdate = true;
    }

    if (bPerformUpdate) UpdateWindow();
  }
}

void RenderWindow::WheelEvent(QWheelEvent *event) {
  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    float fZoom = event->delta()/1000.0f;
    SetTranslationDelta(FLOATVECTOR3(0,0,fZoom),true);
  } else {
    int iZoom = event->delta()/120;  // this returns 1 for "most" mice if the wheel is turned one "click"
    m_Renderer->SetSliceDepth(eWinMode, int(m_Renderer->GetSliceDepth(eWinMode))+iZoom);
  }
  UpdateWindow();
}

void RenderWindow::KeyPressEvent ( QKeyEvent * event ) {
  if (event->key() == Qt::Key_C) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

    if (eWinMode == AbstrRenderer::WM_3D) {
      m_Renderer->SetRenderCoordArrows(!m_Renderer->GetRenderCoordArrows());
    }
  }

  if (event->key() == Qt::Key_R) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

    if (eWinMode == AbstrRenderer::WM_3D) {
      FLOATMATRIX4 mIdentity;
      m_Renderer->SetRotation(mIdentity);
      m_Renderer->SetTranslation(mIdentity);
      m_mCurrentRotation = mIdentity;
      m_mAccumulatedRotation = mIdentity;
      m_mAccumulatedTranslation = mIdentity;
    }
  }

  if (event->key() == Qt::Key_Space) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    AbstrRenderer::EViewMode eMode = AbstrRenderer::EViewMode((int(m_Renderer->GetViewmode()) + 1) % int(AbstrRenderer::VM_INVALID));
    m_Renderer->SetViewmode(eMode);

    if (eMode == AbstrRenderer::VM_SINGLE) 
      m_Renderer->SetFullWindowmode(eWinMode);
    else
     if (m_Renderer->GetStereo()) {
       m_Renderer->SetStereo(false);
       EmitStereoDisabled();
     }

    SetupArcBall();
    EmitRenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    UpdateWindow();    
  }

  if (event->key() == Qt::Key_X) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    bool bFlipX=false, bFlipY=false;
    m_Renderer->Get2DFlipMode(eWinMode, bFlipX, bFlipY);
    bFlipX = !bFlipX;
    m_Renderer->Set2DFlipMode(eWinMode, bFlipX, bFlipY);
  }

  if (event->key() == Qt::Key_Y) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    bool bFlipX=false, bFlipY=false;
    m_Renderer->Get2DFlipMode(eWinMode, bFlipX, bFlipY);
    bFlipY = !bFlipY;
    m_Renderer->Set2DFlipMode(eWinMode, bFlipX, bFlipY);
  }

  if (event->key() == Qt::Key_M) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    bool bUseMIP=false;
    bUseMIP = !m_Renderer->GetUseMIP(eWinMode);
    m_Renderer->SetUseMIP(eWinMode, bUseMIP);
  }
}

void RenderWindow::CloseEvent(QCloseEvent*) {
  EmitWindowClosing();
}

void RenderWindow::FocusInEvent ( QFocusEvent * event ) {
  if (m_Renderer) m_Renderer->SetTimeSlice(m_iTimeSliceMSecsActive);

  if (event->gotFocus()) EmitWindowActive();
}

void RenderWindow::FocusOutEvent ( QFocusEvent * event ) {
  if (m_Renderer) m_Renderer->SetTimeSlice(m_iTimeSliceMSecsInActive);
  if (!event->gotFocus()) EmitWindowInActive();
}

void RenderWindow::SetupArcBall() {
  if (m_Renderer->GetViewmode() == AbstrRenderer::VM_TWOBYTWO) {
    m_ArcBall.SetWindowSize(m_vWinDim.x/2, m_vWinDim.y/2);

    // find the 3D window
    int i3DWindowIndex = 0;
    for (unsigned int i = 0;i<4;i++) {
      if (m_Renderer->Get2x2Windowmode(AbstrRenderer::ERenderArea(i)) == AbstrRenderer::WM_3D) {
        i3DWindowIndex = i;
        break;
      }
    }
    
    switch (i3DWindowIndex) {
      case 0 : m_ArcBall.SetWindowOffset(0,0); break;
      case 1 : m_ArcBall.SetWindowOffset(m_vWinDim.x/2,0); break;
      case 2 : m_ArcBall.SetWindowOffset(0,m_vWinDim.y/2); break;
      case 3 : m_ArcBall.SetWindowOffset(m_vWinDim.x/2, m_vWinDim.y/2); break;
    }
  } else {
    m_ArcBall.SetWindowSize(m_vWinDim.x, m_vWinDim.y);
    m_ArcBall.SetWindowOffset(0,0);
  }

}


void RenderWindow::ToggleRenderWindowView2x2() {
  if (m_Renderer != NULL) {
    m_Renderer->SetViewmode(AbstrRenderer::VM_TWOBYTWO);
    SetupArcBall();
    EmitRenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    UpdateWindow();
  }
}

void RenderWindow::ToggleRenderWindowViewSingle() {
  if (m_Renderer != NULL) {
    m_Renderer->SetViewmode(AbstrRenderer::VM_SINGLE);
    SetupArcBall();
    EmitRenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    UpdateWindow();
  }
}

void RenderWindow::Cleanup() {
  if (m_Renderer == NULL) return;
  
  m_Renderer->Cleanup();
  m_MasterController.ReleaseVolumerenderer(m_Renderer);
  m_Renderer = NULL;
}

void RenderWindow::CheckForRedraw() {
  if (m_Renderer->CheckForRedraw()) UpdateWindow();
}

void RenderWindow::SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  m_Renderer->SetBlendPrecision(eBlendPrecisionMode); 
}

void RenderWindow::SetPerfMeasures(unsigned int iMinFramerate, unsigned int iLODDelay, unsigned int iActiveTS, unsigned int iInactiveTS) {
  m_iTimeSliceMSecsActive   = iActiveTS;
  m_iTimeSliceMSecsInActive = iInactiveTS;
  m_Renderer->SetPerfMeasures(iMinFramerate, iLODDelay); 
}

bool RenderWindow::CaptureFrame(const std::string& strFilename, bool bPreserveTransparency)
{
  GLFrameCapture f;
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  return f.CaptureSingleFrame(strFilename, bPreserveTransparency);
}


bool RenderWindow::CaptureMIPFrame(const std::string& strFilename, float fAngle, bool bOrtho, bool bFinalFrame, bool bUseLOD,
                                   bool bPreserveTransparency, std::string* strRealFilename)
{
  GLFrameCapture f;
  m_Renderer->SetMIPRotationAngle(fAngle);
  bool bSystemOrtho = m_Renderer->GetOrthoView();
  if (bSystemOrtho != bOrtho) m_Renderer->SetOrthoView(bOrtho);
  m_Renderer->SetMIPLOD(bUseLOD);
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  if (bFinalFrame) { // restore state
    m_Renderer->SetMIPRotationAngle(0.0f);
    if (bSystemOrtho != bOrtho) m_Renderer->SetOrthoView(bSystemOrtho);
  }
  return f.CaptureSequenceFrame(strFilename, bPreserveTransparency, strRealFilename);
}

bool RenderWindow::CaptureSequenceFrame(const std::string& strFilename, 
                                        bool bPreserveTransparency, std::string* strRealFilename)
{
  GLFrameCapture f;
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  return f.CaptureSequenceFrame(strFilename, bPreserveTransparency, strRealFilename);
}

void RenderWindow::SetTranslation(const FLOATMATRIX4& mAccumulatedTranslation) {
  m_mAccumulatedTranslation = mAccumulatedTranslation;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);
  m_ArcBall.SetTranslation(m_mAccumulatedTranslation);
}

void RenderWindow::SetTranslationDelta(const FLOATVECTOR3& trans, bool bPropagate) {
  m_mAccumulatedTranslation.m41 += trans.x;
  m_mAccumulatedTranslation.m42 -= trans.y;
  m_mAccumulatedTranslation.m43 += trans.z;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);
  m_ArcBall.SetTranslation(m_mAccumulatedTranslation);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      if (m_bAbsoluteViewLock) 
        m_vpLocks[0][i]->SetTranslation(m_mAccumulatedTranslation);
      else
        m_vpLocks[0][i]->SetTranslationDelta(trans, false);
    }
  }
}

void RenderWindow::FinalizeRotation(bool bPropagate) {
  m_mAccumulatedRotation = m_mCurrentRotation;
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      m_vpLocks[0][i]->FinalizeRotation(false);
    }
  }
}

void RenderWindow::SetRotation(const FLOATMATRIX4& mAccumulatedRotation, 
                               const FLOATMATRIX4& mCurrentRotation) {
  m_mAccumulatedRotation = mAccumulatedRotation;
  m_mCurrentRotation = mCurrentRotation;

  m_Renderer->SetRotation(m_mCurrentRotation);
}


void RenderWindow::SetRotationDelta(const FLOATMATRIX4& rotDelta, bool bPropagate) {
  m_mCurrentRotation = m_mAccumulatedRotation * rotDelta;
  m_Renderer->SetRotation(m_mCurrentRotation);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      if (m_bAbsoluteViewLock) 
        m_vpLocks[0][i]->SetRotation(m_mAccumulatedRotation, m_mCurrentRotation);
      else
        m_vpLocks[0][i]->SetRotationDelta(rotDelta, false);
    }
  }
}

void RenderWindow::CloneViewState(RenderWindow* other) {
  m_mAccumulatedTranslation = other->m_mAccumulatedTranslation;
  m_mAccumulatedRotation    = other->m_mAccumulatedRotation;  
  m_ArcBall.SetTranslation(other->m_ArcBall.GetTranslation());

  m_Renderer->SetRotation(m_mAccumulatedRotation);
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);
}

void RenderWindow::CloneRendermode(RenderWindow* other) {
  SetRendermode(other->GetRendermode());

  m_Renderer->SetUseLighting(other->m_Renderer->GetUseLighting());
  m_Renderer->SetSampleRateModifier(other->m_Renderer->GetSampleRateModifier()); 
  m_Renderer->SetGlobalBBox(other->m_Renderer->GetGlobalBBox());
  m_Renderer->SetLocalBBox(other->m_Renderer->GetLocalBBox());
  m_Renderer->SetIsosufaceColor(other->m_Renderer->GetIsosufaceColor());
  m_Renderer->SetIsoValue(other->m_Renderer->GetIsoValue());
  m_Renderer->SetCVIsoValue(other->m_Renderer->GetCVIsoValue());
  m_Renderer->SetCVSize(other->m_Renderer->GetCVSize());
  m_Renderer->SetCVContextScale(other->m_Renderer->GetCVContextScale());
  m_Renderer->SetCVBorderScale(other->m_Renderer->GetCVBorderScale());
  m_Renderer->SetCVColor(other->m_Renderer->GetCVColor());
  m_Renderer->SetCV(other->m_Renderer->GetCV());
  m_Renderer->SetCVFocusPos(other->m_Renderer->GetCVFocusPos());
}

void RenderWindow::SetRendermode(AbstrRenderer::ERenderMode eRenderMode, bool bPropagate) {
  m_Renderer->SetRendermode(eRenderMode); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetRendermode(eRenderMode, false);
    }
  }
}

void RenderWindow::SetColors(FLOATVECTOR3 vBackColors[2], FLOATVECTOR4 vTextColor) {
  m_Renderer->SetBackgroundColors(vBackColors); 
  m_Renderer->SetTextColor(vTextColor); 
}

void RenderWindow::SetUseLighting(bool bLighting, bool bPropagate) {
  m_Renderer->SetUseLighting(bLighting);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetUseLighting(bLighting, false);
    }
  }
}

void RenderWindow::SetSampleRateModifier(float fSampleRateModifier, bool bPropagate) {
  m_Renderer->SetSampleRateModifier(fSampleRateModifier); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetSampleRateModifier(fSampleRateModifier, false);
    }
  }
}

void RenderWindow::SetIsoValue(float fIsoVal, bool bPropagate) {
  m_Renderer->SetIsoValue(fIsoVal); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetIsoValue(fIsoVal, false);
    }
  }
}

void RenderWindow::SetCVIsoValue(float fIsoVal, bool bPropagate) {
  m_Renderer->SetCVIsoValue(fIsoVal); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVIsoValue(fIsoVal, false);
    }
  }
}

void RenderWindow::SetCVSize(float fSize, bool bPropagate) {
  m_Renderer->SetCVSize(fSize); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVSize(fSize, false);
    }
  }
}

void RenderWindow::SetCVContextScale(float fScale, bool bPropagate) {
  m_Renderer->SetCVContextScale(fScale); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVContextScale(fScale, false);
    }
  }
}

void RenderWindow::SetCVBorderScale(float fScale, bool bPropagate) {
  m_Renderer->SetCVBorderScale(fScale); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVBorderScale(fScale, false);
    }
  }
}

void RenderWindow::SetGlobalBBox(bool bRenderBBox, bool bPropagate) {
  m_Renderer->SetGlobalBBox(bRenderBBox); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetGlobalBBox(bRenderBBox, false);
    }
  }
}

void RenderWindow::SetLocalBBox(bool bRenderBBox, bool bPropagate) {
  m_Renderer->SetLocalBBox(bRenderBBox); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetLocalBBox(bRenderBBox, false);
    }
  }
}

void RenderWindow::SetIsosufaceColor(const FLOATVECTOR3& vIsoColor, bool bPropagate) {
  m_Renderer->SetIsosufaceColor(vIsoColor); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetIsosufaceColor(vIsoColor, false);
    }
  }
}

void RenderWindow::SetCVColor(const FLOATVECTOR3& vIsoColor, bool bPropagate) {
  m_Renderer->SetCVColor(vIsoColor); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVColor(vIsoColor, false);
    }
  }
}

void RenderWindow::SetCV(bool bDoClearView, bool bPropagate) {
  m_Renderer->SetCV(bDoClearView); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCV(bDoClearView, false);
    }
  }
}

void RenderWindow::SetCVFocusPos(const FLOATVECTOR2& vMousePos, bool bPropagate) {
  m_Renderer->SetCVFocusPos(vMousePos);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVFocusPos(vMousePos, false);
    }
  }
}

void RenderWindow::SetLogoParams(QString strLogoFilename, int iLogoPos) {
  m_Renderer->SetLogoParams(std::string(strLogoFilename.toAscii()), iLogoPos);
}

void RenderWindow::SetAbsoluteViewLock(bool bAbsoluteViewLock) {
  m_bAbsoluteViewLock = bAbsoluteViewLock;
}

size_t RenderWindow::GetDynamicRange() const {
  return m_Renderer->Get1DTrans()->GetSize();
}

FLOATVECTOR3 RenderWindow::GetIsosufaceColor() const {
  return m_Renderer->GetIsosufaceColor();
}

FLOATVECTOR3 RenderWindow::GetCVColor() const {
  return m_Renderer->GetCVColor();
}

void RenderWindow::ResizeRenderer(int width, int height)
{
  m_vWinDim = UINTVECTOR2((unsigned int)width, (unsigned int)height);

  if (m_Renderer != NULL) {
    m_Renderer->Resize(UINTVECTOR2(width, height));
    SetupArcBall();
  }
}

void RenderWindow::PaintRenderer()
{
  if (m_Renderer != NULL && m_bRenderSubsysOK) {
    m_Renderer->Paint();
    if (GetQtWidget()->isActiveWindow()) {
      unsigned int iLevelCount        = m_Renderer->GetCurrentSubFrameCount();
      unsigned int iWorkingLevelCount = m_Renderer->GetWorkingSubFrame();

      unsigned int iBrickCount        = m_Renderer->GetCurrentBrickCount();
      unsigned int iWorkingBrick      = m_Renderer->GetWorkingBrick();

      m_MainWindow->SetRenderProgress(iLevelCount, iWorkingLevelCount, iBrickCount, iWorkingBrick);
    } 
  }
}
