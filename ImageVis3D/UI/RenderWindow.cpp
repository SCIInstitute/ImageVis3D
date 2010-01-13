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
#include <QtGui/QtGui>

#include "RenderWindow.h"

#include "ImageVis3D.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"
#include "../Tuvok/Basics/MathTools.h"

using namespace std;
using namespace tuvok;

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
  m_bRebrickingRequired(false),
  selectedRegionSplitter(REGION_SPLITTER_NONE),
  m_vWinDim(0,0),
  m_vMinSize(vMinSize),
  m_vDefaultSize(vDefaultSize),

  m_eViewMode(VM_SINGLE),
  m_vWinFraction(0.5, 0.5),
  m_eRendererType(eType),
  m_MainWindow((MainWindow*)parent),
  m_iTimeSliceMSecsActive(500),
  m_iTimeSliceMSecsInActive(100),
  initialClickPos(0,0),
  m_viMousePos(0,0),
  m_bAbsoluteViewLock(true),
  m_bCaptureMode(false),
  m_bInvWheel(false),
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

RenderWindow::RegionData* RenderWindow::GetRegionData(const RenderRegion* renderRegion) {
#ifdef TR1_NOT_CONST_CORRECT
  tuvok::RenderRegion *region = const_cast<tuvok::RenderRegion*>(renderRegion);
  RegionDataMap::const_iterator iter = regionDataMap.find(region);
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
  const RenderRegion *region = GetRegionUnderCursor(m_viMousePos);

  if (region) {
    // mouse is over the 3D window
    if (region->is3D() ) {
      m_PlaneAtClick = m_ClipPlane;

      if (event->button() == Qt::RightButton)
        initialClickPos = INTVECTOR2(event->pos().x(), event->pos().y());

      if (event->modifiers() & Qt::ControlModifier &&
          event->button() == Qt::LeftButton) {
        RegionData *regionData = GetRegionData(region);
        regionData->clipArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
      } else if (event->button() == Qt::LeftButton) {
        RegionData *regionData = GetRegionData(region);
        regionData->arcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
        regionData->clipArcBall.Click(UINTVECTOR2(event->pos().x(), event->pos().y()));
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
    RenderRegion *startRegion = GetRegionUnderCursor(initialClickPos);
    FinalizeRotation(startRegion, true);
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
    SetClipRotationDelta(regionData->clipArcBall.Drag(upos).ComputeRotation(),
                         true, region);
    bUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - initialClickPos;
    initialClickPos = m_viMousePos;
    SetClipTranslationDelta(FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                                         float(viPosDelta.y*2) / m_vWinDim.y,
                                         0),
                            true, region);
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
    SetCVFocusPos(m_viMousePos);
  }

  if (rotate) {
    UINTVECTOR2 unsigned_pos(pos.x, pos.y);
    RegionData *regionData = GetRegionData(region);
    SetRotationDelta(regionData->arcBall.Drag(unsigned_pos).ComputeRotation(),
                     true, region);
    bPerformUpdate = true;
  }
  if (translate) {
    INTVECTOR2 viPosDelta = m_viMousePos - initialClickPos;
    initialClickPos = m_viMousePos;
    SetTranslationDelta(FLOATVECTOR3(float(viPosDelta.x*2) / m_vWinDim.x,
                                     float(viPosDelta.y*2) / m_vWinDim.y,0),
                        true, region);
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
      SetClipTranslationDelta(FLOATVECTOR3(fZoom/10.f, fZoom/10.f, 0.f), true,
                              renderRegion);
    } else {
      SetTranslationDelta(FLOATVECTOR3(0,0,fZoom), true, renderRegion);
    }
  } else if (renderRegion->is2D())   {
    // this returns 1 for "most" mice if the wheel is turned one "click"
    int iZoom = event->delta()/120;
    int iNewSliceDepth =
      std::max<int>(0,
                    static_cast<int>(m_Renderer->GetSliceDepth(renderRegion))+iZoom);
    size_t sliceDimension = size_t(renderRegion->windowMode);
    iNewSliceDepth =
      std::min<int>(iNewSliceDepth,
                    m_Renderer->GetDataset().GetDomainSize()[sliceDimension]-1);
    m_Renderer->SetSliceDepth(UINT64(iNewSliceDepth), renderRegion);
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
      SetupArcBall();
      EmitRenderWindowViewChanged(int(GetViewMode()));
      UpdateWindow();
    }
      break;
    case Qt::Key_X :
      if(selectedRegion && selectedRegion->is2D()) {
        bool flipX=false, flipY=false;
        m_Renderer->Get2DFlipMode(flipX, flipY, selectedRegion);
        flipX = !flipX;
        m_Renderer->Set2DFlipMode(flipX, flipY, selectedRegion);
      }
      break;
    case Qt::Key_Y :
      if(selectedRegion && selectedRegion->is2D()) {
      bool flipX=false, flipY=false;
      m_Renderer->Get2DFlipMode(flipX, flipY, selectedRegion);
      flipY = !flipY;
      m_Renderer->Set2DFlipMode(flipX, flipY, selectedRegion);
      }
      break;
    case Qt::Key_M :
      if(selectedRegion && selectedRegion->is2D()) {
      bool useMIP = !m_Renderer->GetUseMIP(selectedRegion);
      m_Renderer->SetUseMIP(useMIP, selectedRegion);
      }
      break;
    case Qt::Key_A : {
      RegionData *regionData = GetRegionData(selectedRegion);
      regionData->arcBall.SetUseTranslation(
                                      !regionData->arcBall.GetUseTranslation());
    }
      break;
    case Qt::Key_PageDown : SetTranslationDelta(FLOATVECTOR3(0,0,0.01f), true,
                                                selectedRegion);
      break;

    case Qt::Key_PageUp : SetTranslationDelta(FLOATVECTOR3(0,0,-0.01f), true,
                                              selectedRegion);
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
  f.x = MathTools::Clamp(f.x, 0, 1);
  f.y = MathTools::Clamp(f.y, 0, 1);

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
  m_MasterController.ReleaseVolumerenderer(m_Renderer);
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
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  return f.CaptureSingleFrame(strFilename, bPreserveTransparency);
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
  m_Renderer->ScheduleCompleteRedraw(); // things changed, so need to redraw.
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  if (bFinalFrame) { // restore state
    m_Renderer->SetMIPRotationAngle(0.0f);
    if (bSystemOrtho != bOrtho) m_Renderer->SetOrthoView(bSystemOrtho);
  }
  return f.CaptureSequenceFrame(strFilename, bPreserveTransparency, strRealFilename);
}

bool RenderWindow::CaptureSequenceFrame(const std::string& strFilename,
                                        bool bPreserveTransparency,
                                        std::string* strRealFilename)
{
  GLFrameCapture f;
  ForceRepaint();
  ForceRepaint(); // make sure we have the same results in the front and in the backbuffer
  return f.CaptureSequenceFrame(strFilename, bPreserveTransparency, strRealFilename);
}

void RenderWindow::SetTranslation(const FLOATMATRIX4& mAccumulatedTranslation,
                                  RenderRegion *renderRegion) {
  m_mAccumulatedTranslation = mAccumulatedTranslation;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation, renderRegion);
  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(m_mAccumulatedTranslation);
  Controller::Instance().Provenance("translation", "translate");
}

 void RenderWindow::SetTranslationDelta(const FLOATVECTOR3& trans, bool bPropagate,
                                        RenderRegion *renderRegion) {
  m_mAccumulatedTranslation.m41 += trans.x;
  m_mAccumulatedTranslation.m42 -= trans.y;
  m_mAccumulatedTranslation.m43 += trans.z;
  m_Renderer->SetTranslation(m_mAccumulatedTranslation, renderRegion);
  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(m_mAccumulatedTranslation);

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
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               renderRegion);
      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetTranslation(m_mAccumulatedTranslation, otherRegion);
      else
        m_vpLocks[0][i]->SetTranslationDelta(trans, false, otherRegion);
    }
  }
}

void RenderWindow::FinalizeRotation(const RenderRegion *region, bool bPropagate) {
  m_mAccumulatedRotation = m_Renderer->GetRotation(region);
  // Reset the clip matrix we'll apply; the state is already stored/applied in
  // the ExtendedPlane instance.
  m_mCurrentClipRotation = FLOATMATRIX4();
  m_mAccumulatedClipRotation = m_mCurrentClipRotation;
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               region);
      m_vpLocks[0][i]->FinalizeRotation(otherRegion, false);
    }
  }
  Controller::Instance().Provenance("rotation", "rotate?");
}

void RenderWindow::SetRotation(const FLOATMATRIX4& mAccumulatedRotation,
                               const FLOATMATRIX4& mCurrentRotation,
                               RenderRegion *region) {
  m_mAccumulatedRotation = mAccumulatedRotation;
  m_Renderer->SetRotation(mCurrentRotation, region);
}


void RenderWindow::SetRotationDelta(const FLOATMATRIX4& rotDelta, bool bPropagate,
                                    RenderRegion *region) {
  const FLOATMATRIX4 newRotation = m_mAccumulatedRotation * rotDelta;
  m_Renderer->SetRotation(newRotation, region);

  if(m_Renderer->ClipPlaneLocked()) {
    SetClipRotationDelta(rotDelta, bPropagate, region);
  }

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      RenderRegion *otherRegion = GetCorrespondingRenderRegion(m_vpLocks[0][i],
                                                               region);

      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetRotation(m_mAccumulatedRotation, newRotation,
                                     otherRegion);
      else
        m_vpLocks[0][i]->SetRotationDelta(rotDelta, false, otherRegion);
    }
  }
}

void RenderWindow::SetClipPlane(const ExtendedPlane &p,
                                RenderRegion *renderRegion) {
  m_ClipPlane = p;
  m_Renderer->SetClipPlane(m_ClipPlane, renderRegion);
}

// Applies the given rotation matrix to the clip plane.
// Basically, we're going to translate the plane back to the origin, do the
// rotation, and then push the plane back out to where it should be.  This
// avoids any sort of issues w.r.t. rotating about the wrong point.
void RenderWindow::SetClipRotationDelta(const FLOATMATRIX4& rotDelta,
                                        bool bPropagate,
                                        RenderRegion *renderRegion)
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

  SetClipPlane(rotated, renderRegion);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {

      RenderRegion *otherRegion = GetCorrespondingRenderRegion(*iter, renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(m_ClipPlane, otherRegion);
      } else {
        (*iter)->SetClipRotationDelta(rotDelta, false, otherRegion);
      }
    }
  }
}

// Translates the clip plane by the given vector, projected along the clip
// plane's normal.
void RenderWindow::SetClipTranslationDelta(const FLOATVECTOR3 &trans,
                                           bool bPropagate,
                                           RenderRegion *renderRegion)
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
  SetClipPlane(translated, renderRegion);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {

      RenderRegion *otherRegion = GetCorrespondingRenderRegion(*iter, renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(m_ClipPlane, otherRegion);
      } else {
        (*iter)->SetClipTranslationDelta(trans, false, otherRegion);
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
  m_mAccumulatedTranslation = other->m_mAccumulatedTranslation;
  m_mAccumulatedRotation    = other->m_mAccumulatedRotation;
  m_mAccumulatedClipRotation = other->m_mAccumulatedClipRotation;
  m_mAccumulatedClipTranslation = other->m_mAccumulatedClipTranslation;

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      RegionData *otherData = other->GetRegionData(other->renderRegions[i][j]);
      RegionData *data = GetRegionData(renderRegions[i][j]);
      *data = *otherData;
    }

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

void RenderWindow::SetCVFocusPos(const INTVECTOR2& viMousePos, bool bPropagate) {
  m_Renderer->SetCVFocusPos(viMousePos);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVFocusPos(viMousePos, false);
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
  if (m_Renderer != NULL && m_bRenderSubsysOK) {
    m_Renderer->Paint();
    if (GetQtWidget()->isActiveWindow()) {
      unsigned int iLevelCount        = m_Renderer->GetCurrentSubFrameCount();
      unsigned int iWorkingLevelCount = m_Renderer->GetWorkingSubFrame();

      unsigned int iBrickCount        = m_Renderer->GetCurrentBrickCount();
      unsigned int iWorkingBrick      = m_Renderer->GetWorkingBrick();

      unsigned int iMinLODIndex       = m_Renderer->GetMinLODIndex();

      m_MainWindow->SetRenderProgressAnUpdateInfo(iLevelCount, iWorkingLevelCount,
                                      iBrickCount, iWorkingBrick, iMinLODIndex, this);
    }
  }

 PaintOverlays();
}

void RenderWindow::ResetRenderingParameters()
{
  FLOATMATRIX4 mIdentity;
  m_Renderer->SetRotation(mIdentity);
  m_Renderer->SetTranslation(mIdentity);
  m_mAccumulatedRotation = mIdentity;
  m_mAccumulatedTranslation = mIdentity;
  m_mAccumulatedClipTranslation = mIdentity;
  m_mCurrentClipRotation = mIdentity;
  m_mAccumulatedClipRotation = mIdentity;
  SetClipPlane(PLANE<float>(0,0,1,0));
}
