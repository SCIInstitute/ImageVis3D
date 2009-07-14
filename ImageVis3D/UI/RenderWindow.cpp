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
#include "../Tuvok/Controller/Controller.h"
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
  m_bCaptureMode(false),
  m_bInvWheel(false),
  m_SavedClipLocked(true)
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
  AbstrRenderer::EWindowMode eWinMode =
    m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) /
                                     FLOATVECTOR2(m_vWinDim));
  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    m_PlaneAtClick = m_ClipPlane;

    if (event->button() == Qt::RightButton)
      m_viRightClickPos = INTVECTOR2(event->pos().x(), event->pos().y());

    if (event->modifiers() & Qt::ControlModifier &&
        event->button() == Qt::LeftButton) {
      m_ClipArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
    } else if (event->button() == Qt::LeftButton) {
      m_ArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
      m_ClipArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
    }
  }
}

void RenderWindow::MouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) FinalizeRotation(true);
}

// Qt callback; just interprets the event and passes it on to the appropriate
// ImageVis3D handler.
void RenderWindow::MouseMoveEvent(QMouseEvent *event)
{
  m_viMousePos = INTVECTOR2(event->pos().x(), event->pos().y());
  AbstrRenderer::EWindowMode eWinMode =
    m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) /
                                     FLOATVECTOR2(m_vWinDim));

  bool clip = event->modifiers() & Qt::ControlModifier;
  bool clearview = event->modifiers() & Qt::ShiftModifier;
  bool rotate = event->buttons() & Qt::LeftButton;
  bool translate = event->buttons() & Qt::RightButton;

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D) {
    bool bPerformUpdate = false;

    if(clip) {
      bPerformUpdate = MouseMoveClip(m_viMousePos, rotate, translate);
    } else {
      bPerformUpdate = MouseMove3D(m_viMousePos, clearview, rotate, translate);
    }

    if (bPerformUpdate) UpdateWindow();
  }
}

// A mouse movement which should only affect the clip plane.
bool RenderWindow::MouseMoveClip(INTVECTOR2 pos, bool rotate, bool translate)
{
  bool bUpdate = false;
  if (rotate) {
    UINTVECTOR2 upos(static_cast<UINT32>(pos.x),
                     static_cast<UINT32>(pos.y));
    SetClipRotationDelta(m_ClipArcBall.Drag(upos).ComputeRotation(),true);
    bUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - m_viRightClickPos;
    m_viRightClickPos = m_viMousePos;
    SetClipTranslationDelta(FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                            float(viPosDelta.y*2) / m_vWinDim.y,0),true);
    bUpdate = true;
  }
  return bUpdate;
}

// Move the mouse by the given amount.  Flags tell which rendering parameters
// should be affected by the mouse movement.
bool RenderWindow::MouseMove3D(INTVECTOR2 pos, bool clearview, bool rotate,
                               bool translate)
{
  bool bPerformUpdate = false;

  if (m_Renderer->GetRendermode() == AbstrRenderer::RM_ISOSURFACE &&
      m_Renderer->GetCV() && clearview) {
    SetCVFocusPos(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
  }

  if (rotate) {
    UINTVECTOR2 unsigned_pos(pos.x, pos.y);
    SetRotationDelta(m_ArcBall.Drag(unsigned_pos).ComputeRotation(),true);
    bPerformUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - m_viRightClickPos;
    m_viRightClickPos = m_viMousePos;
    SetTranslationDelta(FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                                     float(viPosDelta.y*2) / m_vWinDim.y,0),
                        true);
    bPerformUpdate = true;
  }
  return bPerformUpdate;
}

void RenderWindow::WheelEvent(QWheelEvent *event) {
  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    float fZoom = ((m_bInvWheel) ? -1 : 1) * event->delta()/1000.0f;

    // User can hold control to modify only the clip plane.  Note however that
    // if the plane is locked to the volume, we'll end up translating the plane
    // regardless of whether or not control is held.
    if(event->modifiers() & Qt::ControlModifier) {
      SetClipTranslationDelta(FLOATVECTOR3(fZoom/10.f,fZoom/10.f,0.f),true);
    } else {
      SetTranslationDelta(FLOATVECTOR3(0,0,fZoom),true);
    }
  } else {
    // this returns 1 for "most" mice if the wheel is turned one "click"
    int iZoom = event->delta()/120;
    int iNewSliceDepth = std::max<int>(0,int(m_Renderer->GetSliceDepth(eWinMode))+iZoom);
    iNewSliceDepth = std::min<int>(iNewSliceDepth, m_Renderer->GetDataset().GetInfo().GetDomainSize()[size_t(eWinMode)]-1);
    m_Renderer->SetSliceDepth(eWinMode, UINT64(iNewSliceDepth));
  }
  UpdateWindow();
}

void RenderWindow::KeyPressEvent ( QKeyEvent * event ) {

  AbstrRenderer::EWindowMode eWinMode =
    m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) /
                                     FLOATVECTOR2(m_vWinDim));
  switch (event->key()) {
    case Qt::Key_F : ToggleFullscreen(); 
                     break;
    case Qt::Key_C : if (eWinMode == AbstrRenderer::WM_3D) {
                       m_Renderer->SetRenderCoordArrows(!m_Renderer->GetRenderCoordArrows());
                     }
                     break;
    case Qt::Key_P : m_Renderer->Set2DPlanesIn3DView(!m_Renderer->Get2DPlanesIn3DView());
                     break;

    case Qt::Key_R : if (eWinMode == AbstrRenderer::WM_3D) {
                        ResetRenderingParameters();
                     }
                     break;
    case Qt::Key_Space : {
                            AbstrRenderer::EViewMode eMode = AbstrRenderer::EViewMode(
                                (int(m_Renderer->GetViewmode()) + 1) % int(AbstrRenderer::VM_INVALID));
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
                         break;
    case Qt::Key_X : {
                        bool bFlipX=false, bFlipY=false;
                        m_Renderer->Get2DFlipMode(eWinMode, bFlipX, bFlipY);
                        bFlipX = !bFlipX;
                        m_Renderer->Set2DFlipMode(eWinMode, bFlipX, bFlipY);
                     }
                     break;

    case Qt::Key_Y : {
                        bool bFlipX=false, bFlipY=false;
                        m_Renderer->Get2DFlipMode(eWinMode, bFlipX, bFlipY);
                        bFlipY = !bFlipY;
                        m_Renderer->Set2DFlipMode(eWinMode, bFlipX, bFlipY);
                     }
                     break;
    case Qt::Key_M : {
                        bool bUseMIP=false;
                        bUseMIP = !m_Renderer->GetUseMIP(eWinMode);
                        m_Renderer->SetUseMIP(eWinMode, bUseMIP);
                     }
                     break;
    case Qt::Key_A : {
                        m_ArcBall.SetUseTranslation(!m_ArcBall.GetUseTranslation());
                     }
                     break;
    case Qt::Key_PageDown : SetTranslationDelta(FLOATVECTOR3(0,0,0.01f),true);
                     break;

    case Qt::Key_PageUp : SetTranslationDelta(FLOATVECTOR3(0,0,-0.01f),true);
                     break;
  }
}

void RenderWindow::CloseEvent(QCloseEvent*) {
  EmitWindowClosing();
  Cleanup();
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
    m_ClipArcBall.SetWindowSize(m_vWinDim.x/2, m_vWinDim.y/2);

    // find the 3D window
    int i3DWindowIndex = 0;
    for (unsigned int i = 0;i<4;i++) {
      if (m_Renderer->Get2x2Windowmode(AbstrRenderer::ERenderArea(i)) == AbstrRenderer::WM_3D) {
        i3DWindowIndex = i;
        break;
      }
    }

    switch (i3DWindowIndex) {
      case 0 : m_ArcBall.SetWindowOffset(0,0);
               m_ClipArcBall.SetWindowOffset(0,0); break;
      case 1 : m_ArcBall.SetWindowOffset(m_vWinDim.x/2,0);
               m_ClipArcBall.SetWindowOffset(m_vWinDim.x/2,0); break;
      case 2 : m_ArcBall.SetWindowOffset(0,m_vWinDim.y/2);
               m_ClipArcBall.SetWindowOffset(0,m_vWinDim.y/2); break;
      case 3 : m_ArcBall.SetWindowOffset(m_vWinDim.x/2, m_vWinDim.y/2);
               m_ClipArcBall.SetWindowOffset(m_vWinDim.x/2, m_vWinDim.y/2); break;
    }
  } else {
    m_ArcBall.SetWindowSize(m_vWinDim.x, m_vWinDim.y);
    m_ArcBall.SetWindowOffset(0,0);

    m_ClipArcBall.SetWindowSize(m_vWinDim.x, m_vWinDim.y);
    m_ClipArcBall.SetWindowOffset(0,0);
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
  if (m_Renderer && m_Renderer->CheckForRedraw()) {
    UpdateWindow();
  }
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
  Controller::Instance().Provenance("translation", "translate");
}

void RenderWindow::SetTranslationDelta(const FLOATVECTOR3& trans, bool bPropagate) {
  m_mAccumulatedTranslation.m41 += trans.x;
  m_mAccumulatedTranslation.m42 -= trans.y;
  m_mAccumulatedTranslation.m43 += trans.z;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);
  m_ArcBall.SetTranslation(m_mAccumulatedTranslation);

  if(GetRenderer()->ClipPlaneLocked()) {
    // We can't use SetClipTranslationDelta, because it forces the clip plane
    // to stay locked along its own normal.  We're not necessarily translating
    // along the clip's normal when doing a regular translation (in fact, it's
    // almost inconceivable that we would be), so we need to do the translation
    // ourself.
    FLOATMATRIX4 translation;
    translation.Translation(trans.x, -trans.y, trans.z);
    SetClipPlane(m_ClipPlane * translation);
  }

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
  // Reset the clip matrix we'll apply; the state is already stored/applied in
  // the ExtendedPlane instance.
  m_mCurrentClipRotation = FLOATMATRIX4();
  m_mAccumulatedClipRotation = m_mCurrentClipRotation;
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      m_vpLocks[0][i]->FinalizeRotation(false);
    }
  }
  Controller::Instance().Provenance("rotation", "rotate?");
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

  if(m_Renderer->ClipPlaneLocked()) {
    SetClipRotationDelta(rotDelta, bPropagate);
  }

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetRotation(m_mAccumulatedRotation, m_mCurrentRotation);
      else
        m_vpLocks[0][i]->SetRotationDelta(rotDelta, false);
    }
  }
}

void RenderWindow::SetClipPlane(const ExtendedPlane &p) {
  m_ClipPlane = p;
  m_Renderer->SetClipPlane(m_ClipPlane);
}

// Applies the given rotation matrix to the clip plane.
// Basically, we're going to translate the plane back to the origin, do the
// rotation, and then push the plane back out to where it should be.  This
// avoids any sort of issues w.r.t. rotating about the wrong point.
void RenderWindow::SetClipRotationDelta(const FLOATMATRIX4& rotDelta,
                                        bool bPropagate)
{
  m_mCurrentClipRotation = m_mAccumulatedClipRotation * rotDelta;
  FLOATVECTOR3 pt = m_PlaneAtClick.Point();
  FLOATMATRIX4 from_pt_to_0, from_0_to_pt;

  // We want to translate the plane to the *dataset's* origin before rotating,
  // not the origin of the entire 3D domain!  The difference is particularly
  // relevant when the clip plane is outside the dataset's domain: the `center'
  // of the plane (*cough*) should rotate about the dataset, not about the
  // plane itself.
  from_pt_to_0.Translation(-m_mAccumulatedTranslation.m41,
                           -m_mAccumulatedTranslation.m42,
                           -m_mAccumulatedTranslation.m43);
  from_0_to_pt.Translation(m_mAccumulatedTranslation.m41,
                           m_mAccumulatedTranslation.m42,
                           m_mAccumulatedTranslation.m43);

  ExtendedPlane rotated = m_PlaneAtClick;
  rotated.Transform(from_pt_to_0 * m_mCurrentClipRotation * from_0_to_pt);

  SetClipPlane(rotated);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {
      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(m_ClipPlane);
      } else {
        (*iter)->SetClipRotationDelta(rotDelta, false);
      }
    }
  }
}

// Translates the clip plane by the given vector, projected along the clip
// plane's normal.
void RenderWindow::SetClipTranslationDelta(const FLOATVECTOR3 &trans,
                                           bool bPropagate)
{
  FLOATMATRIX4 translation;

  // Get the scalar projection of the user's translation along the clip plane's
  // normal.
  float sproj = trans ^ m_ClipPlane.Plane().xyz();
  // The actual translation is along the clip's normal, weighted by the user's
  // translation.
  FLOATVECTOR3 tr = sproj * m_ClipPlane.Plane().xyz();
  translation.Translation(tr.x, tr.y, tr.z);

  ExtendedPlane translated = m_ClipPlane;
  translated.Transform(translation);
  SetClipPlane(translated);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {
      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(m_ClipPlane);
      } else {
        (*iter)->SetClipTranslationDelta(trans, false);
      }
    }
  }
}

void RenderWindow::CloneViewState(RenderWindow* other) {
  m_mAccumulatedTranslation = other->m_mAccumulatedTranslation;
  m_mAccumulatedRotation    = other->m_mAccumulatedRotation;
  m_mAccumulatedClipRotation = other->m_mAccumulatedClipRotation;
  m_mAccumulatedClipTranslation = other->m_mAccumulatedClipTranslation;
  m_ArcBall.SetTranslation(other->m_ArcBall.GetTranslation());
  m_ClipArcBall.SetTranslation(other->m_ClipArcBall.GetTranslation());

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
void RenderWindow::SetClipPlaneEnabled(bool enable, bool bPropagate)
{
  if(enable) {
    m_Renderer->EnableClipPlane();
    // Restore the locking setting which was active when the clip plane was
    // disabled.
    SetClipPlaneRelativeLock(m_SavedClipLocked, bPropagate);
  } else {
    // Disable the clip plane, and then implicitly lock it to the volume.  This
    // means that the clip plane will appear `follow' the volume while it is
    // disabled, which is a bit unintuitive in some sense.
    // However, it might occur that interactions that happen while the clip
    // plane is disabled could cause it to clip the entire volume when
    // re-enabled, which is *very* confusing.  By keeping it locked while
    // disabled, this is prevented, so it's the lesser of the two evils.
    m_SavedClipLocked = m_Renderer->ClipPlaneLocked();
    m_Renderer->DisableClipPlane();
    SetClipPlaneRelativeLock(true, bPropagate);
  }

  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneEnabled(enable, false);
    }
  }
}

void RenderWindow::SetClipPlaneDisplayed(bool bDisp, bool bPropagate)
{
  m_Renderer->ShowClipPlane(bDisp);
  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneDisplayed(bDisp, false);
    }
  }
}

void RenderWindow::SetClipPlaneRelativeLock(bool bLock, bool bPropagate)
{
  m_Renderer->ClipPlaneRelativeLock(bLock);
  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneRelativeLock(bLock, false);
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
    std::ostringstream wsize;
    wsize << m_vWinDim[0] << " " << m_vWinDim[1] << std::ends;
    Controller::Instance().Provenance("window", "resize", wsize.str());
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

      m_MainWindow->SetRenderProgress(iLevelCount, iWorkingLevelCount,
                                      iBrickCount, iWorkingBrick);
    }
  }
}

void RenderWindow::ResetRenderingParameters()
{
  FLOATMATRIX4 mIdentity;
  m_Renderer->SetRotation(mIdentity);
  m_Renderer->SetTranslation(mIdentity);
  m_mCurrentRotation = mIdentity;
  m_mAccumulatedRotation = mIdentity;
  m_mAccumulatedTranslation = mIdentity;
  m_mAccumulatedClipTranslation = mIdentity;
  m_mCurrentClipRotation = mIdentity;
  m_mAccumulatedClipRotation = mIdentity;
  SetClipPlane(PLANE<float>(0,0,1,0));
}
