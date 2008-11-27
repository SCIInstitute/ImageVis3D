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
#include <QtOpenGL/QtOpenGL>
#include <assert.h>
#include <Renderer/GLFrameCapture.h>

RenderWindow::RenderWindow(MasterController& masterController,
                 MasterController::EVolumeRendererType eType,
                 QString dataset,
                 unsigned int iCounter,
                 bool bUseOnlyPowerOfTwo,
                 QGLWidget* glShareWidget,
                 QWidget* parent,
                 Qt::WindowFlags flags) :
  QGLWidget(parent, glShareWidget, flags),
  m_MainWindow((MainWindow*)parent),
  m_Renderer((GLRenderer*)masterController.RequestNewVolumerenderer(eType, bUseOnlyPowerOfTwo)),
  m_MasterController(masterController),
  m_iTimeSliceMSecsActive(500),
  m_iTimeSliceMSecsInActive(100),
  m_bRenderSubsysOK(true),   // be optimistic :-)
  m_strDataset(dataset),
  m_vWinDim(0,0),
  m_bCaptureMode(false)
{  
  SetupArcBall();

  m_strID = tr("[%1] %2").arg(iCounter).arg(m_strDataset);
  setWindowTitle(m_strID);

  m_Renderer->LoadDataset(m_strDataset.toStdString());
  
  // shift the object backwards by two so we are not sitting inside the volume
  m_mAccumulatedTranslation.m43 = -2.0f;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);

  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

RenderWindow::~RenderWindow()
{
  Cleanup();
}

QSize RenderWindow::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize RenderWindow::sizeHint() const
{
  return QSize(400, 400);
}

void RenderWindow::initializeGL()
{
  static bool bFirstTime = true;
  if (bFirstTime) {
    int err = glewInit();
    if (err != GLEW_OK) {
      m_MasterController.DebugOut()->Error("RenderWindow::initializeGL", "Error initializing GLEW: %s",glewGetErrorString(err));
      m_bRenderSubsysOK = false;
      return;
    } else {
      const GLubyte *vendor=glGetString(GL_VENDOR);
      const GLubyte *renderer=glGetString(GL_RENDERER);
      const GLubyte *version=glGetString(GL_VERSION);
      m_MasterController.DebugOut()->Message("RenderWindow::initializeGL", "Starting up GL! Running on a %s %s with OpenGL version %s",vendor, renderer, version);
      m_bRenderSubsysOK = true;
    }

    bFirstTime = false;
  }

  if (m_Renderer == NULL) 
    m_bRenderSubsysOK = false;
  else
    m_bRenderSubsysOK = m_Renderer->Initialize();


  if (!m_bRenderSubsysOK) {
    m_Renderer->Cleanup();
    m_MasterController.ReleaseVolumerenderer(m_Renderer);
    m_Renderer = NULL;
  } 
}

void RenderWindow::paintGL()
{
  if (m_Renderer != NULL && m_bRenderSubsysOK) {
    m_Renderer->Paint();
    if (isActiveWindow()) {
      unsigned int iLevelCount        = m_Renderer->GetCurrentSubFrameCount();
      unsigned int iWorkingLevelCount = m_Renderer->GetWorkingSubFrame();

      unsigned int iBrickCount        = m_Renderer->GetCurrentBrickCount();
      unsigned int iWorkingBrick      = m_Renderer->GetWorkingBrick();

      m_MainWindow->SetRenderProgress(iLevelCount, iWorkingLevelCount, iBrickCount, iWorkingBrick);
    } 
  }
}

void RenderWindow::resizeGL(int width, int height)
{
  m_vWinDim = UINTVECTOR2((unsigned int)width, (unsigned int)height);

  if (m_Renderer != NULL) {
    m_Renderer->Resize(UINTVECTOR2(width, height));
    SetupArcBall();
  }
}

void RenderWindow::mousePressEvent(QMouseEvent *event)
{
  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    if (event->button() == Qt::RightButton) m_viRightClickPos = INTVECTOR2(event->pos().x(), event->pos().y());
    if (event->button() == Qt::LeftButton)  m_ArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
  }
}

void RenderWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) FinalizeRotation(true);
}

void RenderWindow::ToggleHQCaptureMode() {
  m_bCaptureMode = !m_bCaptureMode;
  if (m_bCaptureMode) {
    m_mCaptureStartRotation = m_mAccumulatedRotation;
  } else {
    m_mAccumulatedRotation = m_mCaptureStartRotation;
  }
  m_Renderer->DisableLOD(m_bCaptureMode);
}

void RenderWindow::SetCaptureRotationAngle(float fAngle) {
  FLOATMATRIX4 matRot;
  matRot.RotationY(3.141592653589793238462643383*double(fAngle)/180.0);
  m_mAccumulatedRotation = m_mCaptureStartRotation*matRot;
  m_Renderer->SetRotation(m_mAccumulatedRotation);
  paintGL();
}

void RenderWindow::mouseMoveEvent(QMouseEvent *event)
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

    if (bPerformUpdate) updateGL();
  }
}

void RenderWindow::wheelEvent(QWheelEvent *event) {
  QGLWidget::wheelEvent(event);

  AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));

  // mouse is over the 3D window
  if (eWinMode == AbstrRenderer::WM_3D ) {
    float fZoom = event->delta()/1000.0f;
    SetTranslationDelta(FLOATVECTOR3(0,0,fZoom),true);
  } else {
    int iZoom = event->delta()/120;  // this returns 1 for "most" mice if the wheel is turned one "click"
    m_Renderer->SetSliceDepth(eWinMode, int(m_Renderer->GetSliceDepth(eWinMode))+iZoom);
  }
  updateGL();
}

void RenderWindow::keyPressEvent ( QKeyEvent * event ) {
  QGLWidget::keyPressEvent(event);

  if (event->key() == Qt::Key_Space) {
    AbstrRenderer::EWindowMode eWinMode = m_Renderer->GetWindowUnderCursor(FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim));
    AbstrRenderer::EViewMode eMode = AbstrRenderer::EViewMode((int(m_Renderer->GetViewmode()) + 1) % int(AbstrRenderer::VM_INVALID));
    m_Renderer->SetViewmode(eMode);

    if (eMode == AbstrRenderer::VM_SINGLE) m_Renderer->SetFullWindowmode(eWinMode);

    SetupArcBall();
    emit RenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    updateGL();    
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
    emit RenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    updateGL();
  }
}

void RenderWindow::ToggleRenderWindowViewSingle() {
  if (m_Renderer != NULL) {
    m_Renderer->SetViewmode(AbstrRenderer::VM_SINGLE);
    SetupArcBall();
    emit RenderWindowViewChanged(int(m_Renderer->GetViewmode()));
    updateGL();
  }
}

void RenderWindow::Cleanup() {
  if (m_Renderer == NULL) return;
  
  makeCurrent();
  m_Renderer->Cleanup();
  m_MasterController.ReleaseVolumerenderer(m_Renderer);
  m_Renderer = NULL;
}

void RenderWindow::closeEvent(QCloseEvent *event) {
  QGLWidget::closeEvent(event);

  emit WindowClosing(this);
}

void RenderWindow::focusInEvent ( QFocusEvent * event ) {
  // call superclass method
  QGLWidget::focusInEvent(event);
  if (m_Renderer) m_Renderer->SetTimeSlice(m_iTimeSliceMSecsActive);

  if (event->gotFocus()) {
    emit WindowActive(this);
  }
}

void RenderWindow::focusOutEvent ( QFocusEvent * event ) {
  // call superclass method
  QGLWidget::focusOutEvent(event);
  if (m_Renderer) m_Renderer->SetTimeSlice(m_iTimeSliceMSecsInActive);

  if (!event->gotFocus()) {
    emit WindowInActive(this);
  }
}


void RenderWindow::CheckForRedraw() {
  if (m_Renderer->CheckForRedraw()) {
    makeCurrent();
    update();
  }
}

void RenderWindow::SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  makeCurrent();
  m_Renderer->SetBlendPrecision(eBlendPrecisionMode); 
}

void RenderWindow::SetPerfMeasures(unsigned int iMinFramerate, unsigned int iLODDelay, unsigned int iActiveTS, unsigned int iInactiveTS) {
  makeCurrent();
  m_iTimeSliceMSecsActive   = iActiveTS;
  m_iTimeSliceMSecsInActive = iInactiveTS;
  m_Renderer->SetPerfMeasures(iMinFramerate, iLODDelay); 
}

bool RenderWindow::CaptureFrame(const std::string& strFilename)
{
	GLFrameCapture f;
	makeCurrent();
	return f.CaptureSingleFrame(strFilename);
}

bool RenderWindow::CaptureSequenceFrame(const std::string& strFilename)
{
	GLFrameCapture f;
	makeCurrent();
	return f.CaptureSequenceFrame(strFilename);
}

void RenderWindow::SetTranslationDelta(FLOATVECTOR3 trans, bool bPropagate) {
  m_mAccumulatedTranslation.m41 += trans.x;
  m_mAccumulatedTranslation.m42 -= trans.y;
  m_mAccumulatedTranslation.m43 += trans.z;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation);
  m_ArcBall.SetTranslation(m_mAccumulatedTranslation);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
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

void RenderWindow::SetRotationDelta(FLOATMATRIX4 rotDelta, bool bPropagate) {
  m_mCurrentRotation = m_mAccumulatedRotation * rotDelta;
  m_Renderer->SetRotation(m_mCurrentRotation);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
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

  m_Renderer->SetUseLigthing(other->m_Renderer->GetUseLigthing());
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
  makeCurrent();
  m_Renderer->SetRendermode(eRenderMode); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetRendermode(eRenderMode, false);
    }
  }
}

void RenderWindow::SetColors(FLOATVECTOR3 vBackColors[2], FLOATVECTOR4 vTextColor) {
  makeCurrent();
  m_Renderer->SetBackgroundColors(vBackColors); 
  m_Renderer->SetTextColor(vTextColor); 
}

void RenderWindow::SetUseLigthing(bool bLighting, bool bPropagate) {
  m_Renderer->SetUseLigthing(bLighting); 
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetUseLigthing(bLighting, false);
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

void RenderWindow::SetCVFocusPos(FLOATVECTOR2 vMousePos, bool bPropagate) {
  m_Renderer->SetCVFocusPos(vMousePos);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVFocusPos(vMousePos, false);
    }
  }
}


UINTVECTOR2 RenderWindow::GetDynamicRange() const {
  return UINTVECTOR2(m_Renderer->Get2DTrans()->GetSize());
}

FLOATVECTOR3 RenderWindow::GetIsosufaceColor() const {
  return m_Renderer->GetIsosufaceColor();
}

FLOATVECTOR3 RenderWindow::GetCVColor() const {
  return m_Renderer->GetCVColor();
}