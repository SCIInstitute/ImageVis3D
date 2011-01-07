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

#include "../Tuvok/StdTuvokDefines.h"
#include <cassert>
#include <sstream>
#include <stdexcept>
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility push(default)
#endif
#include <QtGui/QtGui>
#include <QtGui/QMessageBox>
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility pop
#endif

#include "RenderWindow.h"

#include "ImageVis3D.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"
#include "../Tuvok/Basics/MathTools.h"
#include "Basics/tr1.h"

using namespace std;
using namespace tuvok;

std::string RenderWindow::ms_gpuVendorString = "";
UINT32 RenderWindow::ms_iMaxVolumeDims = 0;
bool RenderWindow::ms_b3DTexInDriver = false;

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
  selectedRegionSplitter(REGION_SPLITTER_NONE),
  m_vWinDim(0,0),
  m_vMinSize(vMinSize),
  m_vDefaultSize(vDefaultSize),

  m_eViewMode(VM_SINGLE),
  m_vWinFraction(0.5, 0.5),
  activeRegion(NULL),
  m_eRendererType(eType),
  m_MainWindow((MainWindow*)parent),
  m_iTimeSliceMSecsActive(500),
  m_iTimeSliceMSecsInActive(100),
  initialClickPos(0,0),
  m_viMousePos(0,0),
  m_bAbsoluteViewLock(true),
  m_bInvWheel(false),
  m_RTModeBeforeCapture(AbstrRenderer::RT_INVALID_MODE),
  m_SavedClipLocked(true)
{
  m_strID = "[%1] %2";
  m_strID = m_strID.arg(iCounter).arg(dataset);

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      renderRegions[i][j] = NULL;
}

RenderWindow::~RenderWindow()
{
  Cleanup();
}


void RenderWindow::SetAvoidCompositing(bool bAvoidCompositing) {
  m_Renderer->SetAvoidSeparateCompositing(bAvoidCompositing);
}

bool RenderWindow::GetAvoidCompositing() const {
  return m_Renderer->GetAvoidSeparateCompositing();
}

void RenderWindow::ToggleHQCaptureMode() {
  if (m_RTModeBeforeCapture == AbstrRenderer::RT_CAPTURE) {
    // restore rotation from before the capture process
    SetRotation(GetActiveRenderRegions()[0], m_mCaptureStartRotation);
    m_Renderer->SetRendererTarget(m_RTModeBeforeCapture);
  } else {
    // remember rotation from before the capture process
    m_RTModeBeforeCapture = m_Renderer->GetRendererTarget();
    m_mCaptureStartRotation = m_Renderer->GetRotation(GetActiveRenderRegions()[0]);
    m_Renderer->SetRendererTarget(AbstrRenderer::RT_CAPTURE);
  }
}

void RenderWindow::Translate(const FLOATMATRIX4& mTranslation,
                             RenderRegion *region) {
  if(region == NULL) {
    region = GetFirst3DRegion();
  }
  if(region) {
    SetTranslation(region, mTranslation*m_Renderer->GetTranslation(region));
  }
}

void RenderWindow::Rotate(const FLOATMATRIX4& mRotation, RenderRegion *region) {
  if(region == NULL) {
    region = GetFirst3DRegion();
  }

  if(region) {
    SetRotation(region, mRotation*m_Renderer->GetRotation(region));
  }
}

void RenderWindow::SetCaptureRotationAngle(float fAngle) {
  FLOATMATRIX4 matRot;
  matRot.RotationY(3.141592653589793238462643383*double(fAngle)/180.0);
  matRot = m_mCaptureStartRotation * matRot;
  SetRotation(GetActiveRenderRegions()[0], matRot);
  PaintRenderer();
}

RenderWindow::RegionData*
RenderWindow::GetRegionData(const RenderRegion* const renderRegion) const
{
#ifdef TR1_NOT_CONST_CORRECT
  RenderWindow *cthis = const_cast<RenderWindow*>(this);
  RegionDataMap::const_iterator iter = cthis->regionDataMap.find(renderRegion);
#else
  RegionDataMap::const_iterator iter = regionDataMap.find(renderRegion);
#endif
  if (iter == regionDataMap.end()) {
    // This should never happen if the renderRegion belongs to *this.
    assert(false);
    return NULL;
  }
  return iter->second;
}

RenderWindow::RegionSplitter RenderWindow::GetRegionSplitter(INTVECTOR2 pos) const
{
  switch (m_eViewMode) {
    case VM_SINGLE   : return REGION_SPLITTER_NONE;
    case VM_TWOBYTWO :
      {
        const int halfWidth = regionSplitterWidth/2;
        const INTVECTOR2 splitPoint(m_vWinFraction * FLOATVECTOR2(m_vWinDim));
        const bool isVertical   = abs(pos.x - splitPoint.x) <= halfWidth;
        const bool isHorizontal = abs(pos.y - splitPoint.y) <= halfWidth;

        if (isVertical && isHorizontal) return REGION_SPLITTER_BOTH_2x2;
        if (isVertical)                 return REGION_SPLITTER_VERTICAL_2x2;
        if (isHorizontal)               return REGION_SPLITTER_HORIZONTAL_2x2;
        return REGION_SPLITTER_NONE;
      }
      break;
    default : return REGION_SPLITTER_NONE;
  }
}

void RenderWindow::MousePressEvent(QMouseEvent *event)
{
  activeRegion = GetRegionUnderCursor(m_viMousePos);

  if (activeRegion) {
    // mouse is over the 3D window
    if (activeRegion->is3D() ) {
      SetPlaneAtClick(m_ClipPlane);

      if (event->button() == Qt::RightButton)
        initialClickPos = INTVECTOR2(event->pos().x(), event->pos().y());

      if (event->button() == Qt::LeftButton) {
        RegionData *regionData = GetRegionData(activeRegion);
        regionData->clipArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
        if ( !(event->modifiers() & Qt::ControlModifier) ) {
          regionData->arcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
        }
      }
    }
  } else { // Probably clicked on a region separator.
    selectedRegionSplitter = GetRegionSplitter(m_viMousePos);
    if (selectedRegionSplitter != REGION_SPLITTER_NONE) {
      initialClickPos = INTVECTOR2(event->pos().x(), event->pos().y());
    }
  }
}

void RenderWindow::MouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    if (activeRegion)
      FinalizeRotation(activeRegion, true);
  }

  selectedRegionSplitter = REGION_SPLITTER_NONE;

  RenderRegion *region = GetRegionUnderCursor(m_viMousePos);
  UpdateCursor(region, m_viMousePos, false);
}

// Qt callback; just interprets the event and passes it on to the appropriate
// ImageVis3D handler.
void RenderWindow::MouseMoveEvent(QMouseEvent *event)
{
  m_viMousePos = INTVECTOR2(event->pos().x(), event->pos().y());

  RenderRegion *region = GetRegionUnderCursor(m_viMousePos);

  bool clip = event->modifiers() & Qt::ControlModifier;
  bool clearview = event->modifiers() & Qt::ShiftModifier;
  bool rotate = event->buttons() & Qt::LeftButton;
  bool translate = event->buttons() & Qt::RightButton;

  if (selectedRegionSplitter != REGION_SPLITTER_NONE) {
    region = NULL;
  }

  UpdateCursor(region, m_viMousePos, translate);

  // mouse is over the 3D window
  if (region && region->is3D()) {
    bool bPerformUpdate = false;

    if(clip) {
      bPerformUpdate = MouseMoveClip(m_viMousePos, rotate, translate, region);
    } else {
      bPerformUpdate = MouseMove3D(m_viMousePos, clearview, rotate, translate,
                                   region);
    }

    if (bPerformUpdate) UpdateWindow();
  } else if ( (selectedRegionSplitter != REGION_SPLITTER_NONE) &&
            (event->buttons() & (Qt::LeftButton|Qt::RightButton)) ) {
    FLOATVECTOR2 frac = FLOATVECTOR2(m_viMousePos) / FLOATVECTOR2(m_vWinDim);
    FLOATVECTOR2 winFraction = WindowFraction2x2();
    if (selectedRegionSplitter == REGION_SPLITTER_HORIZONTAL_2x2 ||
        selectedRegionSplitter == REGION_SPLITTER_BOTH_2x2) {
      winFraction.y = frac.y;
    }
    if (selectedRegionSplitter == REGION_SPLITTER_VERTICAL_2x2 ||
        selectedRegionSplitter == REGION_SPLITTER_BOTH_2x2) {
      winFraction.x = frac.x;
    }
    SetWindowFraction2x2(winFraction);
    SetupArcBall();
  }
}

// A mouse movement which should only affect the clip plane.
bool RenderWindow::MouseMoveClip(INTVECTOR2 pos, bool rotate, bool translate,
                                 RenderRegion *region)
{
  bool bUpdate = false;
  if (rotate) {
    UINTVECTOR2 upos(static_cast<UINT32>(pos.x),
                     static_cast<UINT32>(pos.y));
    RegionData *regionData = GetRegionData(region);
    SetClipRotationDelta(region,
                         regionData->clipArcBall.Drag(upos).ComputeRotation(),
                         true, true);
    regionData->clipArcBall.Click(UINTVECTOR2(pos.x, pos.y));
    bUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - initialClickPos;
    initialClickPos = m_viMousePos;
    SetClipTranslationDelta(region,
                            FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                                         float(viPosDelta.y*2) / m_vWinDim.y,
                                         0),
                            true, true);
    bUpdate = true;
  }
  return bUpdate;
}

// Move the mouse by the given amount.  Flags tell which rendering parameters
// should be affected by the mouse movement.
bool RenderWindow::MouseMove3D(INTVECTOR2 pos, bool clearview, bool rotate,
                               bool translate, RenderRegion *region)
{
  bool bPerformUpdate = false;

  if (m_Renderer->GetRendermode() == AbstrRenderer::RM_ISOSURFACE &&
      m_Renderer->GetCV() && clearview) {
    SetCVFocusPos(region, m_viMousePos);
  }

  if (rotate) {
    UINTVECTOR2 unsigned_pos(pos.x, pos.y);
    RegionData *regionData = GetRegionData(region);
    SetRotationDelta(region,
                     regionData->arcBall.Drag(unsigned_pos).ComputeRotation(),
                     true);

    regionData->arcBall.Click(UINTVECTOR2(pos.x, pos.y));
    bPerformUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - initialClickPos;
    initialClickPos = m_viMousePos;
    SetTranslationDelta(region,
                        FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                                     float(viPosDelta.y*2) / m_vWinDim.y,0),
                        true);
    bPerformUpdate = true;
  }
  return bPerformUpdate;
}

void RenderWindow::WheelEvent(QWheelEvent *event) {
  RenderRegion *renderRegion = GetRegionUnderCursor(m_viMousePos);
  if (renderRegion == NULL)
    return;

  // mouse is over the 3D window
  if (renderRegion->is3D()) {
    float fZoom = ((m_bInvWheel) ? -1 : 1) * event->delta()/1000.0f;

    // User can hold control to modify only the clip plane.  Note however that
    // if the plane is locked to the volume, we'll end up translating the plane
    // regardless of whether or not control is held.
    if(event->modifiers() & Qt::ControlModifier) {
      SetClipTranslationDelta(renderRegion,
                              FLOATVECTOR3(fZoom/10.f, fZoom/10.f, 0.f), true, true);
    } else {
      SetTranslationDelta(renderRegion, FLOATVECTOR3(0,0,fZoom), true);
    }
  } else if (renderRegion->is2D())   {
    int iZoom = event->delta() > 0 ? 1 : event->delta() < 0 ? -1 : 0;
    int iNewSliceDepth =
      std::max<int>(0,
                    static_cast<int>(m_Renderer->GetSliceDepth(renderRegion))+iZoom);
    size_t sliceDimension = size_t(renderRegion->windowMode);
    iNewSliceDepth =
      std::min<int>(iNewSliceDepth,
                    m_Renderer->GetDataset().GetDomainSize()[sliceDimension]-1);
    m_Renderer->SetSliceDepth(renderRegion, UINT64(iNewSliceDepth));
  }
  UpdateWindow();
}

RenderRegion* RenderWindow::GetRegionUnderCursor(INTVECTOR2 vPos) const {
  if (vPos.x < 0 || vPos.y < 0)
      return NULL;
  vPos.y = m_vWinDim.y - vPos.y;
  for (size_t i=0; i < GetActiveRenderRegions().size(); ++i) {
    if (GetActiveRenderRegions()[i]->ContainsPoint(UINTVECTOR2(vPos)))
      return GetActiveRenderRegions()[i];
  }
  return NULL;
}

void RenderWindow::UpdateCursor(const RenderRegion *region,
                                INTVECTOR2 pos, bool translate) {
  if (region == NULL) { // We are likely dealing with a splitter
    if (selectedRegionSplitter == REGION_SPLITTER_NONE) { // else cursor already set.
      RegionSplitter hoveredRegionSplitter = GetRegionSplitter(pos);
      switch (hoveredRegionSplitter) {
        case REGION_SPLITTER_HORIZONTAL_2x2:
          GetQtWidget()->setCursor(Qt::SplitVCursor);
          break;
        case REGION_SPLITTER_VERTICAL_2x2:
          GetQtWidget()->setCursor(Qt::SplitHCursor);
          break;
        case REGION_SPLITTER_BOTH_2x2:
          GetQtWidget()->setCursor(Qt::SizeAllCursor);
          break;
        default: ; //Do nothing.
      };
    }
  } else {
    if (translate && region->is3D())
      GetQtWidget()->setCursor(Qt::ClosedHandCursor);
    else
      GetQtWidget()->unsetCursor();
  }
}

void RenderWindow::KeyPressEvent ( QKeyEvent * event ) {

  RenderRegion *selectedRegion = GetRegionUnderCursor(m_viMousePos);

  switch (event->key()) {
    case Qt::Key_F :
      ToggleFullscreen();
      break;
    case Qt::Key_C :
      m_Renderer->SetRenderCoordArrows(!m_Renderer->GetRenderCoordArrows());
      break;
    case Qt::Key_P :
      m_Renderer->Set2DPlanesIn3DView(!m_Renderer->Get2DPlanesIn3DView());
      break;
    case Qt::Key_R :
      ResetRenderingParameters();
      break;
    case Qt::Key_S:
      try {
        FLOATVECTOR3 loc = m_Renderer->Pick(UINTVECTOR2(m_viMousePos));
        OTHER("pick location: %g %g %g", loc[0], loc[1], loc[2]);
      } catch(const std::runtime_error& err) {
        T_ERROR("Pick failed: %s", err.what());
      }
      break;
    case Qt::Key_Space : {
      if (selectedRegion == NULL)
        break;

      EViewMode newViewMode = EViewMode((int(GetViewMode()) + 1) % int(VM_INVALID));
      vector<RenderRegion*> newRenderRegions;

      if (newViewMode == VM_SINGLE) {
        newRenderRegions.push_back(selectedRegion);
      } else {
        if (m_Renderer->GetStereo()) {
          m_Renderer->SetStereo(false);
          EmitStereoDisabled();
        }
        if (newViewMode == VM_TWOBYTWO) {
          for (size_t i=0; i < 4; ++i)
            newRenderRegions.push_back(renderRegions[i][selected2x2Regions[i]]);
        }
      }

      SetViewMode(newRenderRegions, newViewMode);
    }
      break;
    case Qt::Key_X :
      if(selectedRegion && selectedRegion->is2D()) {
        bool flipX=false, flipY=false;
        m_Renderer->Get2DFlipMode(selectedRegion, flipX, flipY);
        flipX = !flipX;
        m_Renderer->Set2DFlipMode(selectedRegion, flipX, flipY);
      }
      break;
    case Qt::Key_Y :
      if(selectedRegion && selectedRegion->is2D()) {
      bool flipX=false, flipY=false;
      m_Renderer->Get2DFlipMode(selectedRegion, flipX, flipY);
      flipY = !flipY;
      m_Renderer->Set2DFlipMode(selectedRegion, flipX, flipY);
      }
      break;
    case Qt::Key_M :
      if(selectedRegion && selectedRegion->is2D()) {
      bool useMIP = !m_Renderer->GetUseMIP(selectedRegion);
      m_Renderer->SetUseMIP(selectedRegion, useMIP);
      }
      break;
    case Qt::Key_A : {
      RegionData *regionData = GetRegionData(selectedRegion);
      regionData->arcBall.SetUseTranslation(
                                      !regionData->arcBall.GetUseTranslation());
    }
      break;
    case Qt::Key_PageDown : case Qt::Key_PageUp :
      if (selectedRegion && selectedRegion->is2D()) {
        const size_t sliceDimension = static_cast<size_t>(selectedRegion->windowMode);
        const int currSlice = static_cast<int>(m_Renderer->GetSliceDepth(selectedRegion));
        const int numSlices = m_Renderer->GetDataset().GetDomainSize()[sliceDimension]-1;
        int sliceChange = numSlices / 10;
        if (event->key() == Qt::Key_PageDown)
          sliceChange = -sliceChange;
        int newSliceDepth = MathTools::Clamp(currSlice + sliceChange, 0, numSlices);
        m_Renderer->SetSliceDepth(selectedRegion, UINT64(newSliceDepth));
      }
      else if (selectedRegion && selectedRegion->is3D()) {
        const float zoom = (event->key() == Qt::Key_PageDown) ? 0.01f : -0.01f;
        SetTranslationDelta(selectedRegion, FLOATVECTOR3(0, 0, zoom), true);
      }
      break;
  }
}

void RenderWindow::CloseEvent(QCloseEvent*) {
  this->GetQtWidget()->setEnabled(false);
  this->GetQtWidget()->hide();
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
  for (size_t i=0; i < GetActiveRenderRegions().size(); ++i) {
    const RenderRegion* region = GetActiveRenderRegions()[i];
    RegionData* regionData = GetRegionData(region);

    const UINTVECTOR2 offset(region->minCoord.x, m_vWinDim.y - region->maxCoord.y);
    const UINTVECTOR2 size = region->maxCoord - region->minCoord;

    regionData->arcBall.SetWindowOffset(offset.x, offset.y);
    regionData->clipArcBall.SetWindowOffset(offset.x, offset.y);
    regionData->arcBall.SetWindowSize(size.x, size.y);
    regionData->clipArcBall.SetWindowSize(size.x, size.y);
  }
}

void RenderWindow::SetWindowFraction2x2(FLOATVECTOR2 f) {
  f.x = MathTools::Clamp(f.x, 0.0, 1.0);
  f.y = MathTools::Clamp(f.y, 0.0, 1.0);

  m_vWinFraction = f;
  m_Renderer->ScheduleCompleteRedraw();
  UpdateWindowFraction();
}


void RenderWindow::UpdateWindowFraction() {
  if (GetActiveRenderRegions().size() != 4) {
    return; // something is wrong, should be 4...
  }

  const int halfWidth = regionSplitterWidth/2;

  int verticalSplit = static_cast<int>(m_vWinDim.x*m_vWinFraction.x);
  int horizontalSplit = static_cast<int>(m_vWinDim.y*(1-m_vWinFraction.y));

  // Make sure none of the regions are out of bounds.  This can happen
  // since we add/subtract the divider width.
  if (verticalSplit - halfWidth < 0)
    verticalSplit = halfWidth;
  if (verticalSplit + halfWidth > static_cast<int>(m_vWinDim.x))
    verticalSplit = m_vWinDim.x - halfWidth;

  if (horizontalSplit - halfWidth < 0)
    horizontalSplit = halfWidth;
  if (horizontalSplit + halfWidth > static_cast<int>(m_vWinDim.y))
    horizontalSplit = m_vWinDim.y - halfWidth;

  const std::vector<RenderRegion*> activeRenderRegions = GetActiveRenderRegions();

  activeRenderRegions[0]->minCoord = UINTVECTOR2(0, horizontalSplit+halfWidth);
  activeRenderRegions[0]->maxCoord = UINTVECTOR2(verticalSplit-halfWidth,
                                                 m_vWinDim.y);

  activeRenderRegions[1]->minCoord = UINTVECTOR2(verticalSplit+halfWidth,
                                                 horizontalSplit+halfWidth);
  activeRenderRegions[1]->maxCoord = UINTVECTOR2(m_vWinDim.x, m_vWinDim.y);

  activeRenderRegions[2]->minCoord = UINTVECTOR2(0, 0);
  activeRenderRegions[2]->maxCoord = UINTVECTOR2(verticalSplit-halfWidth,
                                                 horizontalSplit-halfWidth);

  activeRenderRegions[3]->minCoord = UINTVECTOR2(verticalSplit+halfWidth, 0);
  activeRenderRegions[3]->maxCoord = UINTVECTOR2(m_vWinDim.x,
                                                 horizontalSplit-halfWidth);
}

static std::string view_mode(RenderWindow::EViewMode mode) {
  switch(mode) {
    case RenderWindow::VM_SINGLE: return "single"; break;
    case RenderWindow::VM_TWOBYTWO: return "two-by-two"; break;
    case RenderWindow::VM_INVALID: /* fall-through */
    default: return "invalid"; break;
  }
}

RenderRegion3D*
RenderWindow::GetFirst3DRegion() {
  const std::vector<RenderRegion*>& rr = GetActiveRenderRegions();
  for(size_t i=0; i < rr.size(); ++i) {
    if(rr[i]->is3D()) {
      return dynamic_cast<RenderRegion3D*>(rr[i]);
    }
  }
  return NULL;
}

const std::vector<RenderRegion*>&
RenderWindow::GetActiveRenderRegions() const {
  return m_Renderer->GetRenderRegions();
}

void
RenderWindow::SetActiveRenderRegions(
  const std::vector<RenderRegion*>& regions) const
{
  m_Renderer->SetRenderRegions(regions);
}

void RenderWindow::ToggleRenderWindowView2x2() {
  std::vector<RenderRegion*> newRenderRegions;
  if (GetActiveRenderRegions().size() == 4)
    newRenderRegions = GetActiveRenderRegions();
  else {
    //Just use the default 4 regions.
    for (size_t i=0; i < 4; ++i)
      newRenderRegions.push_back(renderRegions[i][selected2x2Regions[i]]);
  }
  SetViewMode(newRenderRegions, VM_TWOBYTWO);
}

void RenderWindow::ToggleRenderWindowViewSingle() {
  std::vector<RenderRegion*> newRenderRegions;
  if (!GetActiveRenderRegions().empty())
    newRenderRegions.push_back(GetActiveRenderRegions()[0]);
  else
    newRenderRegions.push_back(renderRegions[0][selected2x2Regions[0]]);
  SetViewMode(newRenderRegions, VM_SINGLE);
}

void RenderWindow::SetViewMode(const std::vector<RenderRegion*> &newRenderRegions,
                               EViewMode eViewMode)
{
  m_eViewMode = eViewMode;

  if (eViewMode == VM_SINGLE) {
    if (newRenderRegions.size() != 1) {
      T_ERROR("VM_SINGLE view mode expected only a single RenderRegion, not %d.",
              newRenderRegions.size());
    }
    SetActiveRenderRegions(newRenderRegions);

    // Make the single active region full screen.
    GetActiveRenderRegions()[0]->minCoord = UINTVECTOR2(0,0);
    GetActiveRenderRegions()[0]->maxCoord = m_vWinDim;

  } else if (eViewMode == VM_TWOBYTWO) {
    if (newRenderRegions.size() != 4) {
      T_ERROR("VM_TWOBYTWO view mode expected 4 RenderRegions, not %d.",
              newRenderRegions.size());
    }
    SetActiveRenderRegions(newRenderRegions);
    UpdateWindowFraction();
  }

  SetupArcBall();
  m_Renderer->ScheduleCompleteRedraw();
  UpdateWindow();
  EmitRenderWindowViewChanged(int(GetViewMode()));

  Controller::Instance().Provenance("vmode", "viewmode",
                                    view_mode(eViewMode));
}

void RenderWindow::Initialize() {
  // Note that we create the RenderRegions here and not directly in the constructor
  // because we first need the dataset to be loaded so that we can setup the
  // initial slice index.
  for (int i=0; i < MAX_RENDER_REGIONS; ++i) {
    renderRegions[i][0] = new RenderRegion3D();

    int mode = static_cast<int>(RenderRegion::WM_SAGITTAL);
    UINT64 sliceIndex = m_Renderer->GetDataset().GetDomainSize()[mode]/2;
    renderRegions[i][1] = new RenderRegion2D(RenderRegion::WM_SAGITTAL,
                                             sliceIndex);

    mode = static_cast<int>(RenderRegion::WM_AXIAL);
    sliceIndex = m_Renderer->GetDataset().GetDomainSize()[mode]/2;
    renderRegions[i][2] = new RenderRegion2D(RenderRegion::WM_AXIAL,
                                             sliceIndex);

    mode = static_cast<int>(RenderRegion::WM_CORONAL);
    sliceIndex = m_Renderer->GetDataset().GetDomainSize()[mode]/2;
    renderRegions[i][3] = new RenderRegion2D(RenderRegion::WM_CORONAL,
                                             sliceIndex);
  }

  for (int i=0; i < 4; ++i)
    selected2x2Regions[i] = i;

  // initialize to a full 3D window.
  std::vector<RenderRegion*> initialRenderRegions;
  initialRenderRegions.push_back(renderRegions[0][0]);
  SetActiveRenderRegions(initialRenderRegions);

  // initialize region data map now that we have all the render regions
  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      regionDataMap.insert(std::make_pair(renderRegions[i][j], &regionDatas[i][j]));

  SetupArcBall();
}

void RenderWindow::Cleanup() {
  if (m_Renderer == NULL || !m_bRenderSubsysOK) return;

  m_Renderer->Cleanup();
  m_MasterController.ReleaseVolumeRenderer(m_Renderer);
  m_Renderer = NULL;

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      delete renderRegions[i][j];
}

void RenderWindow::CheckForRedraw() {
  if (m_Renderer && m_Renderer->CheckForRedraw()) {
    UpdateWindow();
  }
}

AbstrRenderer::ERenderMode RenderWindow::GetRenderMode() const {
  return m_Renderer->GetRendermode();
}

void RenderWindow::SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  m_Renderer->SetBlendPrecision(eBlendPrecisionMode);
}

void RenderWindow::SetPerfMeasures(unsigned int iMinFramerate, bool bUseAllMeans,
                                   float fScreenResDecFactor, float fSampleDecFactor,
                                   unsigned int iLODDelay, unsigned int iActiveTS,
                                   unsigned int iInactiveTS) {
  m_iTimeSliceMSecsActive   = iActiveTS;
  m_iTimeSliceMSecsInActive = iInactiveTS;
  m_Renderer->SetPerfMeasures(iMinFramerate, bUseAllMeans, fScreenResDecFactor,
                              fSampleDecFactor, iLODDelay);
}

bool RenderWindow::CaptureFrame(const std::string& strFilename,
                                bool bPreserveTransparency)
{
  GLFrameCapture f;
  m_Renderer->SetRendererTarget(AbstrRenderer::RT_CAPTURE);
  while(m_Renderer->CheckForRedraw()) {
    QCoreApplication::processEvents();
    PaintRenderer();
  }
  
  // as the window is double buffered call repaint twice
  ForceRepaint();  ForceRepaint();
  bool rv = f.CaptureSingleFrame(strFilename, bPreserveTransparency);
  m_Renderer->SetRendererTarget(AbstrRenderer::RT_INTERACTIVE);
  return rv;
}


bool RenderWindow::CaptureMIPFrame(const std::string& strFilename, float fAngle,
                                   bool bOrtho, bool bFinalFrame, bool bUseLOD,
                                   bool bPreserveTransparency,
                                   std::string* strRealFilename)
{
  GLFrameCapture f;
  m_Renderer->SetMIPRotationAngle(fAngle);
  bool bSystemOrtho = m_Renderer->GetOrthoView();
  if (bSystemOrtho != bOrtho) m_Renderer->SetOrthoView(bOrtho);
  m_Renderer->SetMIPLOD(bUseLOD);
  if (bFinalFrame) { // restore state
    m_Renderer->SetMIPRotationAngle(0.0f);
    if (bSystemOrtho != bOrtho) m_Renderer->SetOrthoView(bSystemOrtho);
  }
  // as the window is double buffered call repaint twice
  ForceRepaint();  ForceRepaint();
  bool bResult = f.CaptureSequenceFrame(strFilename, bPreserveTransparency, strRealFilename);
  return bResult;
}

bool RenderWindow::CaptureSequenceFrame(const std::string& strFilename,
                                        bool bPreserveTransparency,
                                        std::string* strRealFilename)
{
  GLFrameCapture f;
  m_Renderer->SetRendererTarget(AbstrRenderer::RT_CAPTURE);
  while(m_Renderer->CheckForRedraw()) {
    QCoreApplication::processEvents();
    PaintRenderer();
  }
  // as the window is double buffered call repaint twice
  ForceRepaint();  ForceRepaint();
  bool rv = f.CaptureSequenceFrame(strFilename, bPreserveTransparency,
                                   strRealFilename);
  m_Renderer->SetRendererTarget(AbstrRenderer::RT_INTERACTIVE);
  return rv;
}

void RenderWindow::SetTranslation(RenderRegion *renderRegion,
                                  const FLOATMATRIX4& mAccumulatedTranslation) {
  m_Renderer->SetTranslation(renderRegion, mAccumulatedTranslation);
  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(mAccumulatedTranslation);

  if(m_Renderer->ClipPlaneLocked()) {
    // We want to translate the plane to the *dataset's* origin before rotating,
    // not the origin of the entire 3D domain!  The difference is particularly
    // relevant when the clip plane is outside the dataset's domain: the `center'
    // of the plane (*cough*) should rotate about the dataset, not about the
    // plane itself.
    FLOATMATRIX4 from_pt_to_0, from_0_to_pt;
    from_pt_to_0.Translation(-m_Renderer->GetTranslation(renderRegion).m41,
                             -m_Renderer->GetTranslation(renderRegion).m42,
                             -m_Renderer->GetTranslation(renderRegion).m43);
    from_0_to_pt.Translation(m_Renderer->GetTranslation(renderRegion).m41,
                             m_Renderer->GetTranslation(renderRegion).m42,
                             m_Renderer->GetTranslation(renderRegion).m43);

    m_ClipPlane.Default(false);
    m_ClipPlane.Transform(m_Renderer->GetTranslation(renderRegion) * from_pt_to_0 * regionData->clipRotation[0] * from_0_to_pt, false);
    SetClipPlane(renderRegion, m_ClipPlane);
  }

  Controller::Instance().Provenance("translation", "translate");
}

void RenderWindow::SetTranslationDelta(RenderRegion *renderRegion,
                                       const FLOATVECTOR3& trans, bool bPropagate) {
  FLOATMATRIX4 newTranslation = m_Renderer->GetTranslation(renderRegion);
  newTranslation.m41 += trans.x;
  newTranslation.m42 -= trans.y;
  newTranslation.m43 += trans.z;
  m_Renderer->SetTranslation(renderRegion, newTranslation);
  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(newTranslation);

  if(GetRenderer()->ClipPlaneLocked()) {
    // We can't use SetClipTranslationDelta, because it forces the clip plane
    // to stay locked along its own normal.  We're not necessarily translating
    // along the clip's normal when doing a regular translation (in fact, it's
    // almost inconceivable that we would be), so we need to do the translation
    // ourself.
    FLOATMATRIX4 translation;
    translation.Translation(trans.x, -trans.y, trans.z);
    m_ClipPlane.Transform(translation, false);
    SetClipPlane(renderRegion, m_ClipPlane);
  }

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               renderRegion);
      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetTranslation(otherRegion, newTranslation);
      else
        m_vpLocks[0][i]->SetTranslationDelta(otherRegion, trans, false);
    }
  }
}

void RenderWindow::FinalizeRotation(const RenderRegion *region, bool bPropagate) {
  // Reset the clip matrix we'll apply; the state is already stored/applied in
  // the ExtendedPlane instance.
  RegionData* regionData = GetRegionData(region);
  regionData->clipRotation[0] = FLOATMATRIX4();
  regionData->clipRotation[1] = FLOATMATRIX4();
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               region);
      m_vpLocks[0][i]->FinalizeRotation(otherRegion, false);
    }
  }
  Controller::Instance().Provenance("rotation", "rotate?");
}

void RenderWindow::SetRotation(RenderRegion *region,
                               const FLOATMATRIX4& newRotation) {
  m_Renderer->SetRotation(region, newRotation);
  if(m_Renderer->ClipPlaneLocked()) {
    
    FLOATMATRIX4 from_pt_to_0, from_0_to_pt;

    // We want to translate the plane to the *dataset's* origin before rotating,
    // not the origin of the entire 3D domain!  The difference is particularly
    // relevant when the clip plane is outside the dataset's domain: the `center'
    // of the plane (*cough*) should rotate about the dataset, not about the
    // plane itself.
    from_pt_to_0.Translation(-m_Renderer->GetTranslation(region).m41,
                             -m_Renderer->GetTranslation(region).m42,
                             -m_Renderer->GetTranslation(region).m43);
    from_0_to_pt.Translation(m_Renderer->GetTranslation(region).m41,
                             m_Renderer->GetTranslation(region).m42,
                             m_Renderer->GetTranslation(region).m43);
  
    RegionData* regionData = GetRegionData(region);
    regionData->clipRotation[0] = newRotation;
    m_ClipPlane.Default(false);
    m_ClipPlane.Transform(m_Renderer->GetTranslation(region) * from_pt_to_0 * regionData->clipRotation[0] * from_0_to_pt, false);
    SetClipPlane(region, m_ClipPlane);
  }
}

void RenderWindow::SetRotationDelta(RenderRegion *region,
                                    const FLOATMATRIX4& rotDelta, bool bPropagate) {
  const FLOATMATRIX4 newRotation = m_Renderer->GetRotation(region) * rotDelta;
  m_Renderer->SetRotation(region, newRotation);

  if(m_Renderer->ClipPlaneLocked()) {
    SetClipRotationDelta(region, rotDelta, bPropagate, false);
  }

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               region);

      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetRotation(otherRegion, newRotation);
      else
        m_vpLocks[0][i]->SetRotationDelta(otherRegion, rotDelta, false);
    }
  }
}

void RenderWindow::SetPlaneAtClick(const ExtendedPlane& plane, bool propagate) {
  m_PlaneAtClick = plane;

  if (propagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      m_vpLocks[0][i]->SetPlaneAtClick(m_vpLocks[0][i]->m_ClipPlane, false);
    }
  }
}

void RenderWindow::SetClipPlane(RenderRegion *renderRegion, const ExtendedPlane &p) {
  m_ClipPlane = p;
  m_Renderer->SetClipPlane(renderRegion, m_ClipPlane);
}

// Applies the given rotation matrix to the clip plane.
// Basically, we're going to translate the plane back to the origin, do the
// rotation, and then push the plane back out to where it should be.  This
// avoids any sort of issues w.r.t. rotating about the wrong point.
void RenderWindow::SetClipRotationDelta(RenderRegion *renderRegion,
                                        const FLOATMATRIX4& rotDelta,
                                        bool bPropagate,
                                        bool bSecondary)
{
  RegionData* regionData = GetRegionData(renderRegion);

  regionData->clipRotation[bSecondary ? 1 : 0] = regionData->clipRotation[bSecondary ? 1 : 0] * rotDelta;
 
  FLOATMATRIX4 from_pt_to_0, from_0_to_pt;

  // We want to translate the plane to the *dataset's* origin before rotating,
  // not the origin of the entire 3D domain!  The difference is particularly
  // relevant when the clip plane is outside the dataset's domain: the `center'
  // of the plane (*cough*) should rotate about the dataset, not about the
  // plane itself.
  from_pt_to_0.Translation(-m_Renderer->GetTranslation(renderRegion).m41,
                           -m_Renderer->GetTranslation(renderRegion).m42,
                           -m_Renderer->GetTranslation(renderRegion).m43);
  from_0_to_pt.Translation(m_Renderer->GetTranslation(renderRegion).m41,
                           m_Renderer->GetTranslation(renderRegion).m42,
                           m_Renderer->GetTranslation(renderRegion).m43);

  ExtendedPlane rotated = m_PlaneAtClick;
  rotated.Transform(from_pt_to_0 * regionData->clipRotation[bSecondary ? 1 : 0] * from_0_to_pt, bSecondary);
  SetClipPlane(renderRegion, rotated);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {

      RenderRegion *otherRegion = GetCorrespondingRenderRegion(*iter, renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(otherRegion, m_ClipPlane);
      } else {
        (*iter)->SetClipRotationDelta(otherRegion, rotDelta, bPropagate, bSecondary);
      }
    }
  }
}

// Translates the clip plane by the given vector, projected along the clip
// plane's normal.
void RenderWindow::SetClipTranslationDelta(RenderRegion *renderRegion,
                                           const FLOATVECTOR3 &trans,
                                           bool bPropagate,
                                           bool bSecondary)
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
  translated.Transform(translation, bSecondary);
  SetClipPlane(renderRegion, translated);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {

      RenderRegion *otherRegion = GetCorrespondingRenderRegion(*iter, renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(otherRegion, m_ClipPlane);
      } else {
        (*iter)->SetClipTranslationDelta(otherRegion, trans, bPropagate, bSecondary);
      }
    }
  }
}

RenderRegion*
RenderWindow::GetCorrespondingRenderRegion(const RenderWindow* otherRW,
                                           const RenderRegion* myRR) const {
  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      if (renderRegions[i][j] == myRR)
        return otherRW->renderRegions[i][j];

  // This should always succeed since myRR must be in this->renderRegions.
  assert(false);
  return NULL;
}

void RenderWindow::CloneViewState(RenderWindow* other) {
  m_mAccumulatedClipTranslation = other->m_mAccumulatedClipTranslation;

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      const RenderRegion* otherRegion = other->renderRegions[i][j];
      const RegionData *otherData = other->GetRegionData(otherRegion);
      RegionData *data = GetRegionData(renderRegions[i][j]);
      *data = *otherData;

      m_Renderer->SetRotation(renderRegions[i][j],
                              other->m_Renderer->GetRotation(otherRegion));
      m_Renderer->SetTranslation(renderRegions[i][j],
                                 other->m_Renderer->GetTranslation(otherRegion));

    }
}

void RenderWindow::CloneRendermode(RenderWindow* other) {
  SetRendermode(other->GetRenderMode());

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

bool RenderWindow::GetUseLighting() const {
  return m_Renderer->GetUseLighting();
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
  if (bPropagate) {
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
    // means that the clip plane will appear to `follow' the volume while it is
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

void RenderWindow::SetCVFocusPos(RenderRegion *region, const INTVECTOR2& viMousePos,
                                 bool bPropagate) {
  m_Renderer->SetCVFocusPos(*region, viMousePos);
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVFocusPos(region, viMousePos, false);
    }
  }
}

void RenderWindow::SetTimestep(size_t t, bool propagate)
{
  m_Renderer->Timestep(t);
  if(propagate) {
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetTimestep(t, false);
    }
  }
}

void RenderWindow::SetLogoParams(QString strLogoFilename, int iLogoPos) {
  m_Renderer->SetLogoParams(std::string(strLogoFilename.toAscii()), iLogoPos);
}

void RenderWindow::SetAbsoluteViewLock(bool bAbsoluteViewLock) {
  m_bAbsoluteViewLock = bAbsoluteViewLock;
}

pair<double,double> RenderWindow::GetDynamicRange() const {
  pair<double,double> range = m_Renderer->GetDataset().GetRange();

  // Old UVFs lack a maxmin data block, && will say the min > the max.
  if (range.first > range.second)
    return make_pair(0,double(m_Renderer->Get1DTrans()->GetSize()));
  else
    return range;
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

  if (m_Renderer != NULL && m_bRenderSubsysOK) {
    switch (GetViewMode()) {
      case VM_SINGLE :
        GetActiveRenderRegions()[0]->maxCoord = m_vWinDim;
        break;
      case VM_TWOBYTWO :
        UpdateWindowFraction();
        break;
      default: break; //nothing to do...
    };

    m_Renderer->Resize(UINTVECTOR2(width, height));
    SetupArcBall();
    std::ostringstream wsize;
    wsize << m_vWinDim[0] << " " << m_vWinDim[1] << std::ends;
    Controller::Instance().Provenance("window", "resize", wsize.str());
  }
}

void RenderWindow::PaintRenderer()
{
  if (!m_strMultiRenderGuard.tryLock()) {
    MESSAGE("Rejecting dublicate Paint call");
    return;
  }

  if (m_Renderer != NULL && m_bRenderSubsysOK) {
    if (!m_Renderer->Paint()) {
      static bool bBugUseronlyOnce = true;
      if (bBugUseronlyOnce) {

        if (m_eRendererType == MasterController::OPENGL_2DSBVR) {
          QMessageBox::critical(NULL, "Render error",
                             "The render subsystem is unable to draw the volume"
                             "This normally means ImageVis3D does not support "
                             "your GPU. Please check the debug log "
                             "('Help | Debug Window') for "
                             "errors, and/or use 'Help | Report an Issue' to "
                             "notify the ImageVis3D developers.");      
        } else {
          QMessageBox::critical(NULL, "Render error",
                             "The render subsystem is unable to draw the volume"
                             "This normally means that your driver is "
                             "reporting invalid information about your GPU."
                             "Try switching the renderer into 2D slicing "
                             "mode in the Preferences/Settings.");
        }
        bBugUseronlyOnce = false;
      }
      T_ERROR("m_Renderer->Paint() call failed.");
    }

    if (GetQtWidget()->isActiveWindow()) {
      unsigned int iLevelCount        = m_Renderer->GetCurrentSubFrameCount();
      unsigned int iWorkingLevelCount = m_Renderer->GetWorkingSubFrame();

      unsigned int iBrickCount        = m_Renderer->GetCurrentBrickCount();
      unsigned int iWorkingBrick      = m_Renderer->GetWorkingBrick();

      unsigned int iMinLODIndex       = m_Renderer->GetMinLODIndex();

      m_MainWindow->SetRenderProgressAnUpdateInfo(iLevelCount,
        iWorkingLevelCount, iBrickCount, iWorkingBrick, iMinLODIndex, this);
    }
  }

 PaintOverlays();
 m_strMultiRenderGuard.unlock();
}

void RenderWindow::ResetRenderingParameters()
{
  FLOATMATRIX4 mIdentity;
  m_mAccumulatedClipTranslation = mIdentity;

  for (int i=0; i < MAX_RENDER_REGIONS; ++i) {
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      regionDatas[i][j].clipRotation[0] = mIdentity;
      regionDatas[i][j].clipRotation[1] = mIdentity;

      RenderRegion *region = renderRegions[i][j];
      m_Renderer->SetRotation(region, mIdentity);
      m_Renderer->SetTranslation(region, mIdentity);
      SetClipPlane(region, ExtendedPlane());
    }
  }
  SetWindowFraction2x2(FLOATVECTOR2(0.5f, 0.5f));
}
