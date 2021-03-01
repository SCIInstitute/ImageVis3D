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
#include "GL/glew.h"
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility push(default)
#endif
#include <QtGui/QtGui>
#include <QtWidgets/QMessageBox>
#if defined(__GNUC__) && defined(DETECTED_OS_LINUX)
# pragma GCC visibility pop
#endif

#include "RenderWindow.h"
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QDesktopWidget>

#include "ImageVis3D.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"
#include "../Tuvok/Renderer/GL/GLFBOTex.h"
#include "../Tuvok/Renderer/GL/GLRenderer.h"
#include "../Tuvok/Renderer/GL/GLTargetBinder.h"
#include "../Tuvok/LuaScripting/LuaScripting.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTuvokTypes.h"

using namespace std;
using namespace tuvok;

std::string RenderWindow::ms_gpuVendorString = "";
uint32_t RenderWindow::ms_iMaxVolumeDims = 0;
bool RenderWindow::ms_b3DTexInDriver = false;
bool RenderWindow::ms_bImageLoadStoreInDriver = false;
bool RenderWindow::ms_bConservativeDepthInDriver = false;

static const float s_fFirstPersonSpeed = 0.001f;

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
  activeRegion(LuaClassInstance::DEFAULT_INSTANCE_ID),
  m_MainWindow((MainWindow*)parent),
  m_MemReg(masterController.LuaScript()),
  m_eRendererType(eType),
  m_iTimeSliceMSecsActive(500),
  m_iTimeSliceMSecsInActive(100),
  m_1DHistScale(0.25f),
  m_2DHistScale(0.75f),
  initialLeftClickPos(0,0),
  initialClickPos(0,0),
  m_viMousePos(0,0),
  m_bAbsoluteViewLock(true),
  m_bInvWheel(false),
  m_bFirstPersonMode(false),
  m_fFirstPersonSpeed(s_fFirstPersonSpeed),
  m_RTModeBeforeCapture(AbstrRenderer::RT_INVALID_MODE),
  m_SavedClipLocked(true)
{
  m_strID = "[%1] %2";
  m_strID = m_strID.arg(iCounter).arg(dataset);
}

RenderWindow::~RenderWindow()
{
  Cleanup();

  // We notify the LuaScripting system because there are instances where this
  // class is destroyed and deleteClass was not called upon it.
  m_MasterController.LuaScript()->notifyOfDeletion(m_LuaThisClass);
}


void RenderWindow::ToggleHQCaptureMode() {
  /// @todo call explicitly through the command system to enable provenance.
  ///       It is not entirely clear that this should be the only function
  ///       called however.
  if (GetRendererTarget() == AbstrRenderer::RT_CAPTURE) {
    EnableHQCaptureMode(false);
  } else {
    EnableHQCaptureMode(true);
  }
}

void RenderWindow::EnableHQCaptureMode(bool enable) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string abstrRenName = GetLuaAbstrRenderer().fqName();

  if (GetRendererTarget() == AbstrRenderer::RT_CAPTURE) {
    // restore rotation from before the capture process
    if (enable == false) {
      /// @fixme Shouldn't this be GetFirst3DRegion?
      SetRotation(GetActiveRenderRegions()[0], m_mCaptureStartRotation);
      SetRendererTarget(m_RTModeBeforeCapture);
    }
  } else {
    // remember rotation from before the capture process
    if (enable == true) {
      m_RTModeBeforeCapture = ss->cexecRet<AbstrRenderer::ERendererTarget>(
          abstrRenName + ".getRendererTarget");
      m_mCaptureStartRotation = GetRotation(GetActiveRenderRegions()[0]);
      SetRendererTarget(AbstrRenderer::RT_CAPTURE);
    }
  }
}

void RenderWindow::SetRendererTarget(AbstrRenderer::ERendererTarget targ)
{
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string abstrRenName = GetLuaAbstrRenderer().fqName();
  ss->cexec(abstrRenName + ".setRendererTarget", targ);
}

FLOATMATRIX4 RenderWindow::GetRotation(LuaClassInstance region)
{
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string rn = region.fqName();
  FLOATMATRIX4 regionRot = ss->cexecRet<FLOATMATRIX4>(rn + ".getRotation4x4");
  return regionRot;
}

FLOATMATRIX4 RenderWindow::GetTranslation(LuaClassInstance region)
{
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string rn = region.fqName();

  FLOATMATRIX4 regionTrans =
      ss->cexecRet<FLOATMATRIX4>(rn + ".getTranslation4x4");
  return regionTrans;
}

void RenderWindow::Translate(const FLOATMATRIX4& translation,
                             LuaClassInstance region) {
  if(region.isValid(m_MasterController.LuaScript()) == false) {
    region = GetFirst3DRegion();
  }
  if(region.isValid(m_MasterController.LuaScript())) {
    /// @todo 4x4 matrix mult -> vector addition.
    FLOATMATRIX4 regionTrans = GetTranslation(region);
    SetTranslation(region, translation * regionTrans);
  }
}

void RenderWindow::Rotate(const FLOATMATRIX4& rotation,
                          LuaClassInstance region) {
  if(region.isValid(m_MasterController.LuaScript()) == false) {
    region = GetFirst3DRegion();
  }
  if(region.isValid(m_MasterController.LuaScript())) {
    FLOATMATRIX4 regionRot = GetRotation(region);
    SetRotation(region, rotation * regionRot);
  }
}

void RenderWindow::SetCaptureRotationAngle(float fAngle) {
  FLOATMATRIX4 matRot;
  matRot.RotationY(3.141592653589793238462643383*double(fAngle)/180.0);
  matRot = m_mCaptureStartRotation * matRot;
  /// @fixme Is the lack of provenance on this next call correct?
  SetRotation(GetActiveRenderRegions()[0], matRot);
  PaintRenderer();
}

RenderWindow::RegionData*
RenderWindow::GetRegionData(LuaClassInstance renderRegion) const
{
  RegionDataMap::const_iterator iter = regionDataMap.find(
      renderRegion.getGlobalInstID());

  if (iter == regionDataMap.end()) {
    // This should never happen if the renderRegion belongs to *this.
    assert(false);
    return NULL;
  }
  return iter->second;
}

uint64_t RenderWindow::GetSliceDepth(LuaClassInstance renderRegion) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<uint64_t>(renderRegion.fqName() + ".getSliceDepth");
}

void RenderWindow::SetSliceDepth(LuaClassInstance renderRegion,
                                 uint64_t newDepth) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(renderRegion.fqName() + ".setSliceDepth", newDepth);
}

bool RenderWindow::IsRegion2D(LuaClassInstance region) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".is2D");
}

bool RenderWindow::IsRegion3D(LuaClassInstance region) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".is3D");
}

bool RenderWindow::DoesRegionContainPoint(LuaClassInstance region,
                                          UINTVECTOR2 pos) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".containsPoint", pos);
}

RenderRegion::EWindowMode RenderWindow::GetRegionWindowMode(
    tuvok::LuaClassInstance region) const {
  /// @todo Remove raw pointer and replace with script call.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  RenderRegion* regPtr = region.getRawPointer<RenderRegion>(ss);
  return regPtr->windowMode;
}

bool RenderWindow::Get2DFlipModeX(LuaClassInstance region) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".get2DFlipModeX");
}

bool RenderWindow::Get2DFlipModeY(LuaClassInstance region) const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".get2DFlipModeY");
}

void RenderWindow::Set2DFlipMode(LuaClassInstance region, bool flipX,
                                 bool flipY) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexec(region.fqName() + ".set2DFlipMode", flipX, flipY);
}

bool RenderWindow::GetUseMIP(LuaClassInstance region) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(region.fqName() + ".getUseMIP");
}

void RenderWindow::SetUseMIP(LuaClassInstance region, bool useMip) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(region.fqName() + ".setUseMIP", useMip);
}

UINTVECTOR2 RenderWindow::GetRegionMinCoord(LuaClassInstance region) const {
  /// @todo Remove raw pointer and replace with script call.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  RenderRegion* regPtr = region.getRawPointer<RenderRegion>(ss);
  return regPtr->minCoord;
}

UINTVECTOR2 RenderWindow::GetRegionMaxCoord(LuaClassInstance region) const {
  /// @todo Remove raw pointer and replace with script call.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  RenderRegion* regPtr = region.getRawPointer<RenderRegion>(ss);
  return regPtr->maxCoord;
}

void RenderWindow::SetRegionMinCoord(LuaClassInstance region,
                                     UINTVECTOR2 minCoord) {
  /// @todo Remove raw pointer and replace with script call.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  RenderRegion* regPtr = region.getRawPointer<RenderRegion>(ss);
  regPtr->minCoord = minCoord;
}
void RenderWindow::SetRegionMaxCoord(LuaClassInstance region,
                                     UINTVECTOR2 maxCoord) {
  /// @todo Remove raw pointer and replace with script call.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  RenderRegion* regPtr = region.getRawPointer<RenderRegion>(ss);
  regPtr->maxCoord = maxCoord;
}

tuvok::AbstrRenderer::ERendererTarget RenderWindow::GetRendererTarget() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<tuvok::AbstrRenderer::ERendererTarget>(
      m_LuaAbstrRenderer.fqName() + ".getRendererTarget");
}

bool RenderWindow::GetClearViewEnabled() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(
      m_LuaAbstrRenderer.fqName() + ".getClearViewEnabled");
}

string RenderWindow::GetRendererClearViewDisabledReason() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<string>(
      m_LuaAbstrRenderer.fqName() + ".getClearViewDisabledReason");
}

float RenderWindow::GetRendererClearViewIsoValue() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<float>(
      m_LuaAbstrRenderer.fqName() + ".getCVIsoValue");
}

float RenderWindow::GetRendererClearViewSize() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<float>(
      m_LuaAbstrRenderer.fqName() + ".getCVSize");
}

float RenderWindow::GetRendererClearViewContextScale() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<float>(
      m_LuaAbstrRenderer.fqName() + ".getCVContextScale");
}

float RenderWindow::GetRendererClearViewBorderScale() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<float>(
      m_LuaAbstrRenderer.fqName() + ".getCVBorderScale");
}

LuaClassInstance RenderWindow::GetRendererDataset() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<LuaClassInstance>(m_LuaAbstrRenderer.fqName() +
                                        ".getDataset");
}

LuaClassInstance RenderWindow::GetRendererTransferFunction1D() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<LuaClassInstance>(m_LuaAbstrRenderer.fqName() +
                                        ".get1DTrans");
}

LuaClassInstance RenderWindow::GetRendererTransferFunction2D() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<LuaClassInstance>(m_LuaAbstrRenderer.fqName() +
                                        ".get2DTrans");
}

void RenderWindow::SetTimeSlice(uint32_t timeSlice) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexec(m_LuaAbstrRenderer.fqName() + ".setTimeSlice", timeSlice);
}

void RenderWindow::ScheduleCompleteRedraw() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".scheduleCompleteRedraw");
}

bool RenderWindow::RendererCheckForRedraw() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<bool>(m_LuaAbstrRenderer.fqName() + ".checkForRedraw");
}

FLOATVECTOR3 RenderWindow::GetBackgroundColor(int i) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<FLOATVECTOR3>(m_LuaAbstrRenderer.fqName() +
                                    ".getBackgroundColor", i);
}

void RenderWindow::SetBackgroundColors(FLOATVECTOR3 vTopColor,
                                       FLOATVECTOR3 vBotColor) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".setBGColors",
            vTopColor, vBotColor);
}

void RenderWindow::SetLightColors(const FLOATVECTOR4& ambient,
                                  const FLOATVECTOR4& diffuse,
                                  const FLOATVECTOR4& specular,
                                  const FLOATVECTOR3& lightDir) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".setLightColors",
            ambient, diffuse, specular, lightDir);
}

FLOATVECTOR4 RenderWindow::GetAmbient() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<FLOATVECTOR4>(m_LuaAbstrRenderer.fqName()+".getAmbient");
}

FLOATVECTOR4 RenderWindow::GetDiffuse() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<FLOATVECTOR4>(m_LuaAbstrRenderer.fqName()+".getDiffuse");
}

FLOATVECTOR4 RenderWindow::GetSpecular() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<FLOATVECTOR4>(m_LuaAbstrRenderer.fqName()+".getSpecular");
}

FLOATVECTOR3 RenderWindow::GetLightDir() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<FLOATVECTOR3>(m_LuaAbstrRenderer.fqName()+".getLightDir");
}

void RenderWindow::SetInterpolant(Interpolant interp) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".setInterpolant", interp);
}

Interpolant RenderWindow::GetInterpolant() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<Interpolant>(m_LuaAbstrRenderer.fqName() 
                                   + ".getInterpolant");
}

void RenderWindow::SetLODLimits(const UINTVECTOR2& vLODLimits) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".setLODLimits", vLODLimits);
}

uint64_t RenderWindow::GetCurrentSubFrameCount() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return static_cast<unsigned int>(ss->cexecRet<uint64_t>(
          rn + ".getCurrentSubFrameCount"));
}

uint32_t RenderWindow::GetWorkingSubFrame() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return static_cast<unsigned int>(ss->cexecRet<uint32_t>(
          rn + ".getWorkingSubFrame"));
}

uint32_t RenderWindow::GetCurrentBrickCount() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return static_cast<unsigned int>(ss->cexecRet<uint32_t>(
          rn + ".getCurrentBrickCount"));
}

uint32_t RenderWindow::GetWorkingBrick() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return static_cast<unsigned int>(ss->cexecRet<uint32_t>(
          rn + ".getWorkingBrick"));
}

uint64_t RenderWindow::GetMinLODIndex() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return static_cast<unsigned int>(ss->cexecRet<uint64_t>(
          rn + ".getMinLODIndex"));
}

void RenderWindow::SetDatasetIsInvalid(bool datasetIsInvalid) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setDatasetIsInvalid", datasetIsInvalid);
}

void RenderWindow::RemoveMeshData(size_t index) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".removeMeshData", index);
}

bool RenderWindow::IsRendererValid() {
  return m_bRenderSubsysOK && m_LuaAbstrRenderer.isValid(
      m_MasterController.LuaScript());
}

bool RenderWindow::SupportsClearView() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".supportsClearView");
}

bool RenderWindow::SupportsMeshes() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".supportsMeshes");
}

void RenderWindow::ScanForNewMeshes() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".scanForNewMeshes");
}

vector<shared_ptr<RenderMesh> > RenderWindow::GetRendererMeshes() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<vector<shared_ptr<RenderMesh> > >(rn + ".getMeshes");
}

void RenderWindow::ClearRendererMeshes() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".clearMeshes");
}

UINTVECTOR2 RenderWindow::GetRendererSize() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<UINTVECTOR2>(rn + ".getSize");
}

bool RenderWindow::GetRendererStereoEnabled() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".getStereoEnabled");
}

float RenderWindow::GetRendererStereoEyeDist() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<float>(rn + ".getStereoEyeDist");
}

float RenderWindow::GetRendererStereoFocalLength() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<float>(rn + ".getStereoFocalLength");
}

bool RenderWindow::GetRendererStereoEyeSwap() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".getStereoEyeSwap");
}

AbstrRenderer::EStereoMode RenderWindow::GetRendererStereoMode() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<AbstrRenderer::EStereoMode >(rn + ".getStereoMode");
}

float RenderWindow::GetRendererSampleRateModifier() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<float>(rn + ".getSampleRateModifier");
}

float RenderWindow::GetRendererFoV() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<float>(rn + ".getFoV");
}

float RenderWindow::GetRendererIsoValue() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<float>(rn + ".getIsoValue");
}

DOUBLEVECTOR3  RenderWindow::GetRendererRescaleFactors() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<DOUBLEVECTOR3>(rn + ".getRescaleFactors");
}

void RenderWindow::SetRendererRescaleFactors(DOUBLEVECTOR3 scale) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setRescaleFactors", scale);
}

bool RenderWindow::GetRendererGlobalBBox() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".getGlobalBBox");
}

bool RenderWindow::GetRendererLocalBBox() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".getLocalBBox");
}

bool RenderWindow::GetRendererClipPlaneEnabled() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".isClipPlaneEnabled");
}

bool RenderWindow::GetRendererClipPlaneLocked() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".isClipPlaneLocked");
}

bool RenderWindow::GetRendererClipPlaneShown() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".isClipPlaneShown");
}

size_t RenderWindow::GetRendererTimestep() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<size_t>(rn + ".getTimestep");
}

UINTVECTOR2 RenderWindow::GetRendererLODLimits() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<UINTVECTOR2>(rn + ".getLODLimits");
}

void RenderWindow::RendererSchedule3DWindowRedraws() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".schedule3DWindowRedraws");
}

void RenderWindow::RendererReloadMesh(size_t index, const shared_ptr<Mesh> m) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".reloadMesh", index, m);
}

FLOATVECTOR3 RenderWindow::GetRendererVolumeAABBExtents() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<FLOATVECTOR3>(rn + ".getVolumeAABBCenter");
}

FLOATVECTOR3 RenderWindow::GetRendererVolumeAABBCenter() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<FLOATVECTOR3>(rn + ".getVolumeAABBExtents");
}

ExtendedPlane RenderWindow::GetRendererClipPlane() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<ExtendedPlane>(rn + ".getClipPlane");
}

bool RenderWindow::RendererCropDataset(const std::wstring& strTempDir, 
                                       bool bKeepOldData) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".cropDataset", strTempDir, bKeepOldData);
}

void RenderWindow::SetRendererStereoEnabled(bool stereo) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setStereoEnabled", stereo);
}

void RenderWindow::SetRendererStereoEyeDist(float fStereoEyeDist) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setStereoEyeDist", fStereoEyeDist);
}

void RenderWindow::SetRendererStereoFocalLength(float fStereoFocalLength) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setStereoFocalLength", fStereoFocalLength);
}

void RenderWindow::SetRendererStereoEyeSwap(bool eyeSwap) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setStereoEyeSwap", eyeSwap);
}

void RenderWindow::SetRendererStereoMode(AbstrRenderer::EStereoMode mode) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setStereoMode", mode);
}

void RenderWindow::RendererInitStereoFrame() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".initStereoFrame");
}

void RenderWindow::RendererToggleStereoFrame() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".toggleStereoFrame");
}

void RenderWindow::RendererSyncStateManager() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".syncStateManager");
}

void RenderWindow::RendererFixedFunctionality() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".fixedFunctionality");
}

RenderWindow::RegionSplitter
RenderWindow::GetRegionSplitter(INTVECTOR2 pos) const
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
  int x = int(event->pos().x()/m_fHighDPIScale+0.5f);  // round
  int y = int(event->pos().y()/m_fHighDPIScale+0.5f);  // round

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  activeRegion = GetRegionUnderCursor(m_viMousePos);

  if (activeRegion.isValid(m_MasterController.LuaScript())) {
    // mouse is over the 3D window
    if (IsRegion3D(activeRegion) ) {
      SetPlaneAtClick(m_ClipPlane);

      if (event->button() == Qt::RightButton) {
        initialClickPos = INTVECTOR2(x, y);
      }

      if (event->button() == Qt::LeftButton) {
        if (m_bFirstPersonMode) 
          QApplication::setOverrideCursor( QCursor(Qt::BlankCursor) );

        initialLeftClickPos = INTVECTOR2(x, y);

        RegionData *regionData = GetRegionData(activeRegion);
        regionData->clipArcBall.Click(UINTVECTOR2(x, y));
        if ( !(event->modifiers() & Qt::ControlModifier) ) {
          regionData->arcBall.Click(UINTVECTOR2(x, y));
        }
      }
    }
  } else { // Probably clicked on a region separator.
    selectedRegionSplitter = GetRegionSplitter(m_viMousePos);
    if (selectedRegionSplitter != REGION_SPLITTER_NONE) {
      initialClickPos = INTVECTOR2(x, y);
    }
  }
}

void RenderWindow::MouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    if (activeRegion.isValid(m_MasterController.LuaScript()))
      FinalizeRotation(activeRegion, true);
  }
  else if (event->button() == Qt::RightButton) {
    if (activeRegion.isValid(m_MasterController.LuaScript()))
      FinalizeTranslation(activeRegion, true);
  }

  QApplication::restoreOverrideCursor();

  selectedRegionSplitter = REGION_SPLITTER_NONE;

  LuaClassInstance region = GetRegionUnderCursor(m_viMousePos);
  UpdateCursor(region, m_viMousePos, false);
}

// Qt callback; just interprets the event and passes it on to the appropriate
// ImageVis3D handler.
void RenderWindow::MouseMoveEvent(QMouseEvent *event) {
  int x = int(event->pos().x()/m_fHighDPIScale+0.5f);  // round
  int y = int(event->pos().y()/m_fHighDPIScale+0.5f);  // round

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  m_viMousePos = INTVECTOR2(x, y);

  LuaClassInstance region = GetRegionUnderCursor(m_viMousePos);

  bool clip = event->modifiers() & Qt::ControlModifier;
  bool clearview = event->modifiers() & Qt::ShiftModifier;
  bool rotate = event->buttons() & Qt::LeftButton;
  bool translate = event->buttons() & Qt::RightButton;

  if (selectedRegionSplitter != REGION_SPLITTER_NONE) {
    region = LuaClassInstance();
  }

  UpdateCursor(region, m_viMousePos, translate);

  // mouse is over the 3D window
  if (region.isValid(m_MasterController.LuaScript()) && IsRegion3D(region)) {
    if (m_bFirstPersonMode) {
      if (event->buttons() & Qt::LeftButton) {
        RotateViewerWithMouse(m_viMousePos-initialLeftClickPos);

        QPoint globalPos = GetQtWidget()->mapToGlobal( GetQtWidget()->geometry().center()) ;
        QCursor::setPos ( globalPos.x(), globalPos.y() );
       
        initialLeftClickPos = INTVECTOR2( int(GetQtWidget()->geometry().center().x()/m_fHighDPIScale+0.5f),
                                          int(GetQtWidget()->geometry().center().y()/m_fHighDPIScale+0.5f));
      }
      UpdateWindow();
    } else {
      bool bPerformUpdate = false;

      if(clip) {
        bPerformUpdate = MouseMoveClip(m_viMousePos, rotate, translate, region);
      } else {
        bPerformUpdate = MouseMove3D(m_viMousePos, clearview, rotate, translate,
                                      region);
      }

      if (bPerformUpdate) UpdateWindow();
    }
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
                                 LuaClassInstance region)
{
  bool bUpdate = false;
  if (rotate) {
    UINTVECTOR2 upos(static_cast<uint32_t>(pos.x),
                     static_cast<uint32_t>(pos.y));
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
                               bool translate, LuaClassInstance region)
{
  bool bPerformUpdate = false;

  if (GetRenderMode() == AbstrRenderer::RM_ISOSURFACE &&
      GetClearViewEnabled() && clearview) {
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
  if (m_bFirstPersonMode) {
    float wheel = event->delta()/100000.0f;
    m_fFirstPersonSpeed = std::max(0.0f,m_fFirstPersonSpeed+wheel);
  } else {
    LuaClassInstance renderRegion = GetRegionUnderCursor(m_viMousePos);
    if (renderRegion.isValid(m_MasterController.LuaScript()) == false)
      return;

    // mouse is over the 3D window
    if (IsRegion3D(renderRegion)) {
      float fZoom = ((m_bInvWheel) ? -1 : 1) * event->delta()/1000.0f;
      MESSAGE("mousewheel click, delta/zoom: %d/%f", event->delta(), fZoom);

      // User can hold control to modify only the clip plane.  Note however that
      // if the plane is locked to the volume, we'll end up translating the plane
      // regardless of whether or not control is held.
      if(event->modifiers() & Qt::ControlModifier) {
        SetClipTranslationDelta(renderRegion,
                                FLOATVECTOR3(fZoom/10.f, fZoom/10.f, 0.f), true, true);
      } else {
        SetTranslationDelta(renderRegion, FLOATVECTOR3(0,0,fZoom), true);
      }
    } else if (IsRegion2D(renderRegion))   {
      int iZoom = event->delta() > 0 ? 1 : event->delta() < 0 ? -1 : 0;
      int iNewSliceDepth =
        std::max<int>(0,
                      static_cast<int>(GetSliceDepth(renderRegion))+iZoom);
      size_t sliceDimension = size_t(GetRegionWindowMode(renderRegion));

      shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
      LuaClassInstance dataset = GetRendererDataset();
      UINT64VECTOR3 domainSize = ss->cexecRet<UINT64VECTOR3>(
          dataset.fqName() + ".getDomainSize", (size_t)0, (size_t)0);
      iNewSliceDepth =
        std::min<int>(iNewSliceDepth, domainSize[sliceDimension]-1);
      SetSliceDepth(renderRegion, uint64_t(iNewSliceDepth));
    }
    UpdateWindow();
  }
}

LuaClassInstance RenderWindow::GetRegionUnderCursor(INTVECTOR2 vPos) const {
  if (vPos.x < 0 || vPos.y < 0)
      return LuaClassInstance();

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  vPos.y = m_vWinDim.y - vPos.y;
  for (size_t i=0; i < GetActiveRenderRegions().size(); ++i) {
    if (DoesRegionContainPoint(GetActiveRenderRegions()[i], UINTVECTOR2(vPos)))
      return GetActiveRenderRegions()[i];
  }
  return LuaClassInstance();
}

void RenderWindow::UpdateCursor(LuaClassInstance region,
                                INTVECTOR2 pos, bool translate) {
  if (region.isValid(m_MasterController.LuaScript()) == false) {
    // We are likely dealing with a splitter
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
    /// @todo Convert to a script call.
    if (translate && IsRegion3D(region))
      GetQtWidget()->setCursor(Qt::ClosedHandCursor);
    else
      GetQtWidget()->unsetCursor();
  }
}


void RenderWindow::RotateViewerWithMouse(const INTVECTOR2& viMouseDelta) {
   
  const int screen = QApplication::desktop()->screenNumber(m_MainWindow);
  const QRect availableRect(QGuiApplication::screens()[screen]->availableGeometry());

  const FLOATVECTOR2 vfMouseDelta(viMouseDelta.x/float(availableRect.width()),
                                  viMouseDelta.y/float(availableRect.height()));

  RotateViewer(FLOATVECTOR3(-vfMouseDelta.x*40.f, -vfMouseDelta.y*40.f, 0.f));
}


void RenderWindow::RotateViewer(const FLOATVECTOR3& vfAngles) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string an = GetLuaAbstrRenderer().fqName();

  FLOATVECTOR3 viewDir =
    ss->cexecRet<FLOATVECTOR3>(an + ".getViewDir");

  FLOATVECTOR3 upVec =
    ss->cexecRet<FLOATVECTOR3>(an + ".getUpDir");

  FLOATVECTOR3 left = upVec % viewDir;

  FLOATMATRIX4 matRotationX, matRotationY, matRotationZ;
  matRotationX.RotationAxis(upVec.normalized(),   (vfAngles.x * M_PI) / 180.0);
  matRotationY.RotationAxis(left.normalized(),    (vfAngles.y * M_PI) / 180.0);
  matRotationZ.RotationAxis(viewDir.normalized(), (vfAngles.z * M_PI) / 180.0);

  viewDir = matRotationX * viewDir * matRotationY;
  upVec   = matRotationZ * upVec * matRotationY;

  ss->cexec(an + ".setViewDir", viewDir);
  ss->cexec(an + ".setUpDir", upVec);
  UpdateWindow();
}


void RenderWindow::MoveViewerWithMouse(const FLOATVECTOR3& vDirection) {
  MoveViewer(vDirection*m_fFirstPersonSpeed);
}

void RenderWindow::MoveViewer(const FLOATVECTOR3& vDirection) {

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string an = GetLuaAbstrRenderer().fqName();

  const FLOATVECTOR3 up =
      ss->cexecRet<FLOATVECTOR3>(an + ".getUpDir");
  const FLOATVECTOR3 view =
      ss->cexecRet<FLOATVECTOR3>(an + ".getViewDir");
  const FLOATVECTOR3 left = up % view;
      
  FLOATVECTOR3 oldPos =
      ss->cexecRet<FLOATVECTOR3>(an + ".getViewPos");

  oldPos = oldPos + vDirection.x * left
                  + vDirection.y * up
                  + vDirection.z * view;

  ss->cexec(an + ".setViewPos", oldPos);
  UpdateWindow();
}

void RenderWindow::SetFirstPersonMode(bool bFirstPersonMode)
{
  m_bFirstPersonMode = bFirstPersonMode;
}

bool RenderWindow::GetFirstPersonMode()
{
  return m_bFirstPersonMode;
}

void RenderWindow::KeyPressEvent ( QKeyEvent * event ) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string rn = m_LuaAbstrRenderer.fqName();

  LuaClassInstance selectedRegion = GetRegionUnderCursor(m_viMousePos);

  /// @todo Add keyboard shortcuts dialog.

  switch (event->key()) {
    case Qt::Key_D :
      ss->cexec(rn + ".cycleDebugViews");
      break;
    case Qt::Key_F :
      m_bFirstPersonMode = !m_bFirstPersonMode;
      break;
    case Qt::Key_C :
      ss->cexec(rn + ".setCoordinateArrowsEnabled",
                !ss->cexecRet<bool>(rn + ".getCoordinateArrowsEnabled"));
      break;
    case Qt::Key_T :
      ss->cexec(rn + ".transfer3DRotationToMIP");
      break;
    case Qt::Key_P :
      if (selectedRegion.isValid(ss))
      {
        ss->cexec(rn + ".set2DPlanesIn3DView",
                  !ss->cexecRet<bool>(rn + ".get2DPlanesIn3DView"),
                  selectedRegion);
      }
      break;
    case Qt::Key_R :
      ResetRenderingParameters();
      break;
    case Qt::Key_Space : {
      if (selectedRegion.isValid(ss) == false)
        break;

      EViewMode newViewMode = EViewMode((int(GetViewMode()) + 1) % int(VM_INVALID));
      vector<LuaClassInstance> newRenderRegions;

      if (newViewMode == VM_SINGLE) {
        newRenderRegions.push_back(selectedRegion);
      } else {
        if (ss->cexecRet<bool>(rn + ".getStereoEnabled")) {
          ss->cexec(rn + ".setStereoEnabled", false);
          EmitStereoDisabled();
        }
        if (newViewMode == VM_TWOBYTWO) {
          for (size_t i=0; i < 4; ++i)
            newRenderRegions.push_back(luaRenderRegions[i][selected2x2Regions[i]]);
        }
      }

      SetViewMode(newRenderRegions, newViewMode);
    }
      break;
    case Qt::Key_X :
      if (   selectedRegion.isValid(ss)
          && IsRegion2D(selectedRegion)) {
        bool flipX = Get2DFlipModeX(selectedRegion);
        flipX = !flipX;
        Set2DFlipMode(selectedRegion, flipX, Get2DFlipModeY(selectedRegion));
      }
      break;
    case Qt::Key_Y :
      if(    selectedRegion.isValid(ss)
          && IsRegion2D(selectedRegion)) {
        bool flipY = Get2DFlipModeY(selectedRegion);
        flipY = !flipY;
        Set2DFlipMode(selectedRegion, Get2DFlipModeX(selectedRegion), flipY);
      }
      break;
    case Qt::Key_M :
      if(    selectedRegion.isValid(ss)
          && IsRegion2D(selectedRegion)) {
        bool useMIP = !GetUseMIP(selectedRegion);
        SetUseMIP(selectedRegion, useMIP);
      }
      break;
    case Qt::Key_A : {
      if (selectedRegion.isValid(ss))
      {
        RegionData *regionData = GetRegionData(selectedRegion);
        regionData->arcBall.SetUseTranslation(
                                      !regionData->arcBall.GetUseTranslation());
      }
    }
      break;
    case Qt::Key_Left : 
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(1,0,0));
      }
      break;
    case Qt::Key_Up : 
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(0,0,1));
      }
      break;
    case Qt::Key_Right : 
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(-1,0,0));
      }
      break;
    case Qt::Key_Down :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(0,0,-1));
      }
      break;
    case Qt::Key_Home : 
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(0,1,0));
      }
      break;
    case Qt::Key_End :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        MoveViewerWithMouse(FLOATVECTOR3(0,-1,0));
      }
      break;
    case Qt::Key_7 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(0,0,-1)); // roll left
      }
      break;
    case Qt::Key_9 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(0,0,1)); // roll right
      }
      break;
    case Qt::Key_4 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(1,0,0)); // pitch left
      }
      break;
    case Qt::Key_6 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(-1,0,0)); // pitch right
      }
      break;
    case Qt::Key_8 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(0,1,0)); // yaw up
      }
      break;
    case Qt::Key_5 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        RotateViewer(FLOATVECTOR3(0,-1,0)); // yaw down
      }
      break;
    case Qt::Key_0 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("key0Pressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_1 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("key1Pressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_2 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("key2Pressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_3 :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("key3Pressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Slash :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keySlashPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Asterisk :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keyAsteriskPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Minus :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keyMinusPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Plus :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keyPlusPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Comma :
    case Qt::Key_Period :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keyCommaOrPeriodPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_Enter :
      if (m_bFirstPersonMode && selectedRegion.isValid(ss) && IsRegion3D(selectedRegion)) {
        try {
          ss->exec("keyEnterPressed()");
        } catch (const tuvok::LuaError&) {}
      }
      break;
    case Qt::Key_PageDown : case Qt::Key_PageUp :
      if (   selectedRegion.isValid(ss)
          && IsRegion2D(selectedRegion)) {
        const size_t sliceDimension = static_cast<size_t>(
            GetRegionWindowMode(selectedRegion));
        const int currSlice = static_cast<int>(GetSliceDepth(selectedRegion));
        LuaClassInstance dataset = GetRendererDataset();
        UINT64VECTOR3 domainSize = ss->cexecRet<UINT64VECTOR3>(
            dataset.fqName() + ".getDomainSize", (size_t)0, (size_t)0);
        const int numSlices = domainSize[sliceDimension]-1;
        int sliceChange = numSlices / 10;
        if (event->key() == Qt::Key_PageDown)
          sliceChange = -sliceChange;
        int newSliceDepth = MathTools::Clamp(currSlice + sliceChange, 0, numSlices);
        SetSliceDepth(selectedRegion, uint64_t(newSliceDepth));
      }
      else if (   selectedRegion.isValid(m_MasterController.LuaScript())
               && IsRegion3D(selectedRegion)) {
        const float zoom = (event->key() == Qt::Key_PageDown) ? 0.01f : -0.01f;
        SetTranslationDelta(selectedRegion, FLOATVECTOR3(0, 0, zoom), true);
      }
      break;
  }
}

void RenderWindow::CloseEvent(QCloseEvent* close) {
  this->GetQtWidget()->setEnabled(false);
  this->GetQtWidget()->lower();
  EmitWindowClosing();
  close->accept();
}

void RenderWindow::FocusInEvent ( QFocusEvent * event ) {
  if (m_LuaAbstrRenderer.isValid(m_MasterController.LuaScript()))
    SetTimeSlice(m_iTimeSliceMSecsActive);
  if (event->gotFocus()) EmitWindowActive();
}

void RenderWindow::FocusOutEvent ( QFocusEvent * event ) {
  if (m_LuaAbstrRenderer.isValid(m_MasterController.LuaScript()))
    SetTimeSlice(m_iTimeSliceMSecsInActive);
  if (!event->gotFocus()) EmitWindowInActive();
}

void RenderWindow::SetupArcBall() {
  for (size_t i=0; i < GetActiveRenderRegions().size(); ++i) {
    LuaClassInstance region = GetActiveRenderRegions()[i];
    RegionData* regionData = GetRegionData(region);

    const UINTVECTOR2 regMin = GetRegionMinCoord(region);
    const UINTVECTOR2 regMax = GetRegionMaxCoord(region);
    const UINTVECTOR2 offset(regMin.x, m_vWinDim.y - regMax.y);
    const UINTVECTOR2 size = regMax - regMin;

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
  ScheduleCompleteRedraw();
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

  const std::vector<LuaClassInstance> activeRenderRegions =
      GetActiveRenderRegions();

  SetRegionMinCoord(activeRenderRegions[0],
                    UINTVECTOR2(0, horizontalSplit+halfWidth));
  SetRegionMaxCoord(activeRenderRegions[0],
                    UINTVECTOR2(verticalSplit-halfWidth, m_vWinDim.y));

  SetRegionMinCoord(activeRenderRegions[1],
                    UINTVECTOR2(verticalSplit+halfWidth,
                                horizontalSplit+halfWidth));
  SetRegionMaxCoord(activeRenderRegions[1],
                    UINTVECTOR2(m_vWinDim.x, m_vWinDim.y));

  SetRegionMinCoord(activeRenderRegions[2],
                    UINTVECTOR2(0,0));
  SetRegionMaxCoord(activeRenderRegions[2],
                    UINTVECTOR2(verticalSplit-halfWidth,
                                horizontalSplit-halfWidth));

  SetRegionMinCoord(activeRenderRegions[3],
                    UINTVECTOR2(verticalSplit+halfWidth, 0));
  SetRegionMaxCoord(activeRenderRegions[3],
                    UINTVECTOR2(m_vWinDim.x, horizontalSplit-halfWidth));
}

LuaClassInstance
RenderWindow::GetFirst3DRegion() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<LuaClassInstance>(GetLuaAbstrRenderer().fqName()
                                        + ".getFirst3DRenderRegion");
}

const std::vector<LuaClassInstance>
RenderWindow::GetActiveRenderRegions() const {
  return m_MasterController.LuaScript()->
      cexecRet<std::vector<LuaClassInstance> >(m_LuaAbstrRenderer.fqName() +
                                               ".getRenderRegions");
}

void RenderWindow::SetActiveRenderRegions(std::vector<LuaClassInstance> regions)
  const
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(GetLuaAbstrRenderer().fqName() + ".setRenderRegions",
            regions);
}

void RenderWindow::ToggleRenderWindowView2x2() {
  std::vector<LuaClassInstance> newRenderRegions;
  if (GetActiveRenderRegions().size() == 4)
    newRenderRegions = GetActiveRenderRegions();
  else {
    //Just use the default 4 regions.
    for (size_t i=0; i < 4; ++i)
      newRenderRegions.push_back(luaRenderRegions[i][selected2x2Regions[i]]);
  }
  SetViewMode(newRenderRegions, VM_TWOBYTWO);
}


bool RenderWindow::SetRenderWindowView3D() {
  std::vector<LuaClassInstance> newRenderRegions;

  for (int i=0; i < MAX_RENDER_REGIONS; ++i) {
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      if (IsRegion3D(luaRenderRegions[i][j])) {
        newRenderRegions.push_back(luaRenderRegions[i][j]);
        SetViewMode(newRenderRegions, RenderWindow::VM_SINGLE);
        return true;
      }
    }
  }
  return false;
}

void RenderWindow::ToggleRenderWindowViewSingle() {
  std::vector<LuaClassInstance> newRenderRegions;
  if (!GetActiveRenderRegions().empty())
    newRenderRegions.push_back(GetActiveRenderRegions()[0]);
  else
    newRenderRegions.push_back(luaRenderRegions[0][selected2x2Regions[0]]);
  SetViewMode(newRenderRegions, VM_SINGLE);
}

void
RenderWindow::SetViewMode(const std::vector<LuaClassInstance> &newRenderRegions,
                          EViewMode eViewMode)
{
  m_eViewMode = eViewMode;

  if (eViewMode == VM_SINGLE) {
    if (newRenderRegions.size() != 1) {
      T_ERROR("VM_SINGLE view mode expected only a single RenderRegion, not %d.",
              newRenderRegions.size());
    }
    SetActiveRenderRegions(newRenderRegions);

    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    LuaClassInstance firstRenRegion = GetActiveRenderRegions()[0];
    RenderRegion* regPtr = firstRenRegion.getRawPointer<RenderRegion>(ss);

    /// @fixme Is the following code correct? At the top of RenderRegion.h it
    /// says:
    // NOTE: client code should never directly modify a RenderRegion. Instead,
    // modifications should be done through the tuvok API so that tuvok is aware
    // of these changes.
    regPtr->minCoord = UINTVECTOR2(0,0);
    regPtr->maxCoord = m_vWinDim;

  } else if (eViewMode == VM_TWOBYTWO) {
    if (newRenderRegions.size() != 4) {
      T_ERROR("VM_TWOBYTWO view mode expected 4 RenderRegions, not %d.",
              newRenderRegions.size());
    }
    SetActiveRenderRegions(newRenderRegions);
    UpdateWindowFraction();
  }

  SetupArcBall();
  ScheduleCompleteRedraw();
  UpdateWindow();
  EmitRenderWindowViewChanged(int(GetViewMode()));
}

void RenderWindow::Initialize() {
  // Note that we create the RenderRegions here and not directly in the constructor
  // because we first need the dataset to be loaded so that we can setup the
  // initial slice index.

  // NOTE: Since this function is called from our derived class' constructor
  // we can generate lua instances and have them associated with the call
  // to the constructor (ensures we do not have to hit undo multiple times
  // in order to undo the creation of the render window).

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  for (int i=0; i < MAX_RENDER_REGIONS; ++i) {
    luaRenderRegions[i][0] = ss->cexecRet<LuaClassInstance>(
        "tuvok.renderRegion3D.new", GetLuaAbstrRenderer());

    int mode = static_cast<int>(RenderRegion::WM_SAGITTAL);
    LuaClassInstance dataset = GetRendererDataset();
    UINT64VECTOR3 domainSize = ss->cexecRet<UINT64VECTOR3>(
        dataset.fqName() + ".getDomainSize", (size_t)0, (size_t)0);
    uint64_t sliceIndex = domainSize[mode]/2;
    luaRenderRegions[i][1] = ss->cexecRet<LuaClassInstance>(
            "tuvok.renderRegion2D.new",
            mode,
            sliceIndex, GetLuaAbstrRenderer());

    mode = static_cast<int>(RenderRegion::WM_AXIAL);
    sliceIndex = domainSize[mode]/2;
    luaRenderRegions[i][2] = ss->cexecRet<LuaClassInstance>(
            "tuvok.renderRegion2D.new",
            mode,
            sliceIndex, GetLuaAbstrRenderer());

    mode = static_cast<int>(RenderRegion::WM_CORONAL);
    sliceIndex = domainSize[mode]/2;
    luaRenderRegions[i][3] = ss->cexecRet<LuaClassInstance>(
            "tuvok.renderRegion2D.new",
            mode,
            sliceIndex, GetLuaAbstrRenderer());
  }

  for (int i=0; i < 4; ++i)
    selected2x2Regions[i] = i;

  std::vector<LuaClassInstance> initialRenderRegions;
  initialRenderRegions.push_back(luaRenderRegions[0][0]);
  ss->cexec(GetLuaAbstrRenderer().fqName() + ".setRenderRegions",
            initialRenderRegions);

  // initialize region data map now that we have all the render regions
  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      regionDataMap.insert(std::make_pair(
          luaRenderRegions[i][j].getGlobalInstID(),
          &regionDatas[i][j]));

  SetupArcBall();
}

void RenderWindow::Cleanup() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (m_LuaAbstrRenderer.isValid(ss) == false)
    return;

  ss->cexec(m_LuaAbstrRenderer.fqName() + ".cleanup");
  m_MasterController.ReleaseVolumeRenderer(m_LuaAbstrRenderer);
  m_LuaAbstrRenderer.invalidate();

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      ss->cexec("deleteClass", luaRenderRegions[i][j]);
}

void RenderWindow::CheckForRedraw() {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (m_LuaAbstrRenderer.isValid(ss) && RendererCheckForRedraw()) {
    UpdateWindow();
  }
}

AbstrRenderer::ERenderMode RenderWindow::GetRenderMode() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  return ss->cexecRet<AbstrRenderer::ERenderMode>(m_LuaAbstrRenderer.fqName()
                                                  + ".getRenderMode");
}

void RenderWindow::SetBlendPrecision(
    AbstrRenderer::EBlendPrecision eBlendPrecisionMode) {
  m_MasterController.LuaScript()->cexec(
      GetLuaAbstrRenderer().fqName() + ".setBlendPrecision",
      eBlendPrecisionMode);
}

void RenderWindow::SetPerfMeasures(unsigned int iMinFramerate,
                                   bool bRenderLowResIntermediateResults,
                                   float fScreenResDecFactor,
                                   float fSampleDecFactor,
                                   unsigned int iLODDelay,
                                   unsigned int iActiveTS,
                                   unsigned int iInactiveTS) {
  m_iTimeSliceMSecsActive   = iActiveTS;
  m_iTimeSliceMSecsInActive = iInactiveTS;
  m_MasterController.LuaScript()->cexec(m_LuaAbstrRenderer.fqName() +
                                        ".setPerfMeasures",
                                        iMinFramerate,
                                        bRenderLowResIntermediateResults,
                                        fScreenResDecFactor,
                                        fSampleDecFactor, iLODDelay);
}

bool RenderWindow::CaptureFrame(const std::wstring& strFilename,
                                bool bPreserveTransparency)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());

  ss->setTempProvDisable(true);

  GLFrameCapture f;
  AbstrRenderer::ERendererTarget mode = GetRendererTarget();
  FLOATVECTOR3 color[2] = {GetBackgroundColor(0),
                           GetBackgroundColor(1)};
  FLOATVECTOR3 black[2] = {FLOATVECTOR3(0,0,0), FLOATVECTOR3(0,0,0)};
  if (bPreserveTransparency) SetBackgroundColors(black[0],black[1]);

  SetRendererTarget(AbstrRenderer::RT_CAPTURE);
  while(RendererCheckForRedraw()) {
    QCoreApplication::processEvents();
    PaintRenderer();
  }
  // as the window is double buffered call repaint twice
  ForceRepaint();  ForceRepaint();

  bool rv = f.CaptureSingleFrame(strFilename, bPreserveTransparency);
  SetRendererTarget(mode);
  if (bPreserveTransparency) SetBackgroundColors(color[0],color[1]);

  ss->setTempProvDisable(false);

  return rv;
}

bool RenderWindow::CaptureSubframe(const std::wstring& strFilename) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->setTempProvDisable(true);

  bool bPreserveTransparency = false;
  GLFrameCapture f;
  bool rv = f.CaptureSingleFrame(strFilename, bPreserveTransparency);

  ss->setTempProvDisable(false);

  return rv;
}


bool RenderWindow::CaptureMIPFrame(const std::wstring& strFilename, float fAngle,
                                   bool bOrtho, bool bFinalFrame, bool bUseLOD,
                                   bool bPreserveTransparency,
                                   std::wstring* strRealFilename)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();

  ss->setTempProvDisable(true);

  GLFrameCapture f;
  ss->cexec(rn + ".setMIPRotationAngle", fAngle);
  bool bSystemOrtho = ss->cexecRet<bool>(rn + ".getOrthoViewEnabled");
  if (bSystemOrtho != bOrtho) ss->cexec(rn + ".setOrthoViewEnabled", bOrtho);
  ss->cexec(rn + ".setMIPLODEnabled", bUseLOD);
  if (bFinalFrame) { // restore state
    ss->cexec(rn + ".setMIPRotationAngle", 0.0f);
    if (bSystemOrtho != bOrtho)
      ss->cexec(rn + ".setOrthoViewEnabled", bSystemOrtho);
  }
  // as the window is double buffered call repaint twice
  ForceRepaint();  ForceRepaint();

  std::wstring strSequenceName = SysTools::FindNextSequenceName(strFilename);
  if (strRealFilename) (*strRealFilename) = strSequenceName;

  ss->setTempProvDisable(false);

  return f.CaptureSingleFrame(strSequenceName, bPreserveTransparency);
}

bool RenderWindow::CaptureSequenceFrame(const std::wstring& strFilename,
                                        bool bPreserveTransparency,
                                        std::wstring* strRealFilename)
{
  std::wstring strSequenceName = SysTools::FindNextSequenceName(strFilename);
  if (strRealFilename) (*strRealFilename) = strSequenceName;
  return CaptureFrame(strSequenceName, bPreserveTransparency); 
}

void RenderWindow::SetTranslation(LuaClassInstance renderRegion,
                                  FLOATMATRIX4 accumulatedTranslation) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->setTempProvDisable(true);
  ss->cexec(renderRegion.fqName()+".setTranslation4x4",accumulatedTranslation);
  ss->setTempProvDisable(false);

  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(accumulatedTranslation);

  updateClipPlaneTransform(renderRegion);
}

void RenderWindow::SetTranslationDelta(LuaClassInstance renderRegion,
                                       const FLOATVECTOR3& trans, bool
                                       bPropagate) {
  FLOATMATRIX4 newTranslation = GetTranslation(renderRegion);
  newTranslation.m41 += trans.x;
  newTranslation.m42 -= trans.y;
  newTranslation.m43 += trans.z;

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->setTempProvDisable(true);
  ss->cexec(renderRegion.fqName() + ".setTranslation4x4", newTranslation);
  ss->setTempProvDisable(false);

  RegionData *regionData = GetRegionData(renderRegion);
  regionData->arcBall.SetTranslation(newTranslation);

  updateClipPlaneTransform(renderRegion);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(
          m_vpLocks[0][i], renderRegion);
      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetTranslation(otherRegion, newTranslation);
      else
        m_vpLocks[0][i]->SetTranslationDelta(otherRegion, trans, false);
    }
  }
}

void RenderWindow::FinalizeRotation(LuaClassInstance region, bool bPropagate) {
  // Reset the clip matrix we'll apply; the state is already stored/applied in
  // the ExtendedPlane instance.
  // setRotationAs4x4

  // We group the functions inside of SetProvRotationAndClip together so that
  // one undo command will undo them all.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->beginCommandGroup();
  SetProvRotationAndClip(region, GetRotation(region));
  ss->endCommandGroup();

  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(
          m_vpLocks[0][i], region);
      m_vpLocks[0][i]->FinalizeRotation(otherRegion, false);
    }
  }
}

void RenderWindow::FinalizeTranslation(LuaClassInstance region, bool bPropagate)
{
  // Call our registered Lua function so that undo of both the clipping plane
  // and the rotation occur in one step.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->beginCommandGroup();
  SetProvTransAndClip(region, GetTranslation(region));
  ss->endCommandGroup();

  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(
          m_vpLocks[0][i], region);
      m_vpLocks[0][i]->FinalizeTranslation(otherRegion, false);
    }
  }
}

void RenderWindow::SetRotation(LuaClassInstance region,
                               FLOATMATRIX4 newRotation) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string rn = m_LuaAbstrRenderer.fqName();

  // Temporarily disable provenance. We don't want to record every single
  // rotation command, only the final rotation command.
  /// @todo should wrap in try catch so that setTempProvDisable always gets
  ///       called.
  ss->setTempProvDisable(true);
  ss->cexec(region.fqName() + ".setRotation4x4", newRotation);
  ss->setTempProvDisable(false);

  updateClipPlaneTransform(region);
}

void RenderWindow::SetRotationDelta(LuaClassInstance region,
                                    const FLOATMATRIX4& rotDelta,
                                    bool bPropagate) {
  const FLOATMATRIX4 newRotation = GetRotation(region) * rotDelta;
  string rn = m_LuaAbstrRenderer.fqName();

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->setTempProvDisable(true);
  ss->cexec(region.fqName() + ".setRotation4x4", newRotation);
  ss->setTempProvDisable(false);

  updateClipPlaneTransform(region);

  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(
          m_vpLocks[0][i], region);

      if (m_bAbsoluteViewLock)
        m_vpLocks[0][i]->SetRotation(otherRegion, newRotation);
      else
        m_vpLocks[0][i]->SetRotationDelta(otherRegion, rotDelta, false);
    }
  }
}

void RenderWindow::updateClipPlaneTransform(LuaClassInstance region)
{
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string renName = m_LuaAbstrRenderer.fqName();

  // Test whether the clipping plane is locked. If it is, then utilize the
  // model->world transform of the volume. Otherwise, just utilize the clip
  // to world matrix.

  /// @todo Handle inverted plane normal. A simple inverted boolean will
  ///       suffice.
  if(ss->cexecRet<bool>(renName + ".isClipPlaneLocked")) {
    // (clip space ->) object space -> world space
    FLOATMATRIX4 r = computeClipToVolToWorldTransform(region);

    ExtendedPlane plane;
    plane.Transform(r, false);
    SetClipPlane(region, plane);
  } else {
    RegionData* regionData = GetRegionData(region);
    FLOATMATRIX4 cs = regionData->toClipSpace.inverse();
    ExtendedPlane plane;
    plane.Transform(cs, false);
    SetClipPlane(region, plane);
  }
}

FLOATMATRIX4
RenderWindow::getHomogeneousVolToWorldTrafo(LuaClassInstance region)
{
  FLOATMATRIX4 vr = GetRotation(region);
  FLOATMATRIX4 vt = GetTranslation(region);

  // Create homogeneous volume->world matrix.
  FLOATMATRIX4 vx(vr);
  vx.m41 = vt.m41;
  vx.m42 = vt.m42;
  vx.m43 = vt.m43;

  return vx;
}

FLOATMATRIX4
RenderWindow::computeClipToVolToWorldTransform(LuaClassInstance region)
{
  RegionData* regionData = GetRegionData(region);

  // Create homogeneous volume->world matrix.
  FLOATMATRIX4 vx = getHomogeneousVolToWorldTrafo(region);

  /// @todo We need a homogenous matrix inversion. The inversion below is
  ///       overkill.
  FLOATMATRIX4 ics = regionData->toClipSpace.inverse();

  // (clip space ->) object space -> world space
  return ics * vx;
}

void RenderWindow::SetPlaneAtClick(const ExtendedPlane& plane, bool propagate) {
  m_PlaneAtClick = plane;

  if (propagate) {
    for (size_t i = 0;i<m_vpLocks[0].size();i++) {
      m_vpLocks[0][i]->SetPlaneAtClick(m_vpLocks[0][i]->m_ClipPlane, false);
    }
  }
}

void RenderWindow::SetClipPlane(LuaClassInstance region,
                                const ExtendedPlane &p) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  m_ClipPlane = p;
  ss->setTempProvDisable(true);
  ss->cexec(region.fqName() + ".setClipPlane", m_ClipPlane);
  ss->setTempProvDisable(false);
}

// Applies the given rotation matrix to the clip plane.
// Basically, we're going to translate the plane back to the origin, do the
// rotation, and then push the plane back out to where it should be.  This
// avoids any sort of issues w.r.t. rotating about the wrong point.
void RenderWindow::SetClipRotationDelta(LuaClassInstance renderRegion,
                                        const FLOATMATRIX4& rotDelta,
                                        bool bPropagate,
                                        bool bSecondary)
{
  RegionData* regionData = GetRegionData(renderRegion);
  // Transform the rotation into clipping space preserving transformation
  // order: (R3^-1*R2^-1*R1^-1)^-1 = R1 * R2 * R3. This inversion is performed
  // in computeClipToVolToWorldTransform to leave clipping space and enter 
  // object space.
  regionData->toClipSpace = rotDelta.inverse() * regionData->toClipSpace;
  updateClipPlaneTransform(renderRegion);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::const_iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {
      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(
          *iter,renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(otherRegion, m_ClipPlane);
      } else {
        (*iter)->SetClipRotationDelta(otherRegion, rotDelta, false, bSecondary);
      }
    }
  }
}

// Translates the clip plane by the given vector, projected along the clip
// plane's normal.
void RenderWindow::SetClipTranslationDelta(LuaClassInstance renderRegion,
                                           const FLOATVECTOR3 &trans,
                                           bool bPropagate,
                                           bool bSecondary)
{
  RegionData* regionData = GetRegionData(renderRegion);

  FLOATMATRIX4 translation;
  FLOATMATRIX4 worldToClip =
      computeClipToVolToWorldTransform(renderRegion).inverse();

  // Get the scalar projection of the user's translation along the clip plane's
  // normal. We want to perform this scalar projection in world space.
  float sproj = trans ^ m_ClipPlane.Plane().xyz();
  // The actual translation is along the clip's normal, weighted by the user's
  // translation. Done in clipping space.
  PLANE<float> clipSpacePlane = m_ClipPlane.Plane();
  clipSpacePlane.transform(worldToClip);  // Remember, transformations against
                                          // planes are of the form (M^-1)^T.
                                          // We can't just transform
                                          // m_ClipPlane.Plane().xyz() by
                                          // worldToClip.
  FLOATVECTOR3 clipSpaceNormal = - clipSpacePlane.xyz();
  // NOTE: We could just extract the negative 3rd column from the
  //       regionData->toClipSpace matrix.
  //       But this would make us dependent on the choice of the initial normal
  //       in extended plane (see ms_Plane in ExtendededPlane)
  FLOATVECTOR3 tr = sproj * clipSpaceNormal;
  translation.Translation(tr);
  regionData->toClipSpace = regionData->toClipSpace * translation;

  updateClipPlaneTransform(renderRegion);

  if (bPropagate) {
    for(std::vector<RenderWindow*>::iterator iter = m_vpLocks[0].begin();
        iter != m_vpLocks[0].end(); ++iter) {

      LuaClassInstance otherRegion = GetCorrespondingRenderRegion(*iter, renderRegion);

      if (m_bAbsoluteViewLock) {
        (*iter)->SetClipPlane(otherRegion, m_ClipPlane);
      } else {
        (*iter)->SetClipTranslationDelta(otherRegion, trans, bPropagate, bSecondary);
      }
    }
  }
}

LuaClassInstance
RenderWindow::GetCorrespondingRenderRegion(const RenderWindow* otherRW,
                                           LuaClassInstance myRR) const {
  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j)
      if (luaRenderRegions[i][j].getGlobalInstID() == myRR.getGlobalInstID())
        return otherRW->luaRenderRegions[i][j];

  // This should always succeed since myRR must be in this->renderRegions.
  assert(false);
  return LuaClassInstance();
}

void RenderWindow::CloneViewState(RenderWindow* other) {
  m_mAccumulatedClipTranslation = other->m_mAccumulatedClipTranslation;

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  for (int i=0; i < MAX_RENDER_REGIONS; ++i)
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      const LuaClassInstance otherRegion = other->luaRenderRegions[i][j];
      const RegionData *otherData = other->GetRegionData(otherRegion);
      RegionData *data = GetRegionData(luaRenderRegions[i][j]);
      *data = *otherData;

      ss->cexec(luaRenderRegions[i][j].fqName() + ".setRotation4x4",
                other->GetRotation(otherRegion));
      ss->cexec(luaRenderRegions[i][j].fqName() + ".setTranslation4x4",
                other->GetTranslation(otherRegion));
    }
}

void RenderWindow::CloneRendermode(RenderWindow* other) {
  SetRendermode(other->GetRenderMode());

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".cloneRenderMode",
            other->m_LuaAbstrRenderer);
}

void RenderWindow::SetRendermode(AbstrRenderer::ERenderMode eRenderMode, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_LuaAbstrRenderer.fqName() + ".setRenderMode", eRenderMode);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetRendermode(eRenderMode, false);
    }
  }
}

void RenderWindow::SetColors(FLOATVECTOR3 vTopColor, FLOATVECTOR3 vBotColor,
                             FLOATVECTOR4 vTextColor) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  string abstrRenName = GetLuaAbstrRenderer().fqName();

  /// @todo Composite these two calls into one lua function to ensure they occur
  ///       on the same undo/redo call.
  ss->cexec(abstrRenName + ".setBGColors", vTopColor, vBotColor);
  ss->cexec(abstrRenName + ".setTextColor", vTextColor);
}

void RenderWindow::SetUseLighting(bool bLighting, bool bPropagate) {
  /// @todo Have callback that will toggle the 'lighting' checkbox.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setLightingEnabled", bLighting);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetUseLighting(bLighting, false);
    }
  }
}

bool RenderWindow::GetUseLighting() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<bool>(rn + ".getLightingEnabled");
}

void RenderWindow::SetSampleRateModifier(float fSampleRateModifier,
                                         bool bPropagate) {
  /// @todo Prov: We need a set 'final' sample rate modifier. Otherwise, sample
  ///       rate modifier calls stack up in the undo/redo stacks.
  /// @todo Update sample rate slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setSampleRateModifier", fSampleRateModifier);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetSampleRateModifier(fSampleRateModifier, false);
    }
  }
}

void RenderWindow::SetFoV(float fFoV, bool bPropagate) {
  /// @todo Update FoV slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setFoV", fFoV);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetFoV(fFoV, false);
    }
  }
}

void RenderWindow::SetIsoValue(float fIsoVal, bool bPropagate) {
  /// @todo Provenance: Need final iso value instead of continuous change.
  /// @todo Update isovalue slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setIsoValue", fIsoVal);
  if(bPropagate) {
    LuaClassInstance dataset = GetRendererDataset();
    pair<double,double> range = ss->cexecRet<pair<double,double> >(
        dataset.fqName() + ".getRange");
    // we might not have a valid range (old UVFs, color data).  In that case,
    // use the bit width.
    if(range.second <= range.first) {
      range.first = 0.0;
      double width = static_cast<double>(ss->cexecRet<uint64_t>(
              dataset.fqName() + ".getBitWidth"));
      range.second = std::pow(2.0, width);
    }
    float isoval = MathTools::lerp<double,float>(fIsoVal,
      range.first,range.second, 0.0f,1.0f
    );
    for(size_t i=0; i < m_vpLocks[1].size(); ++i) {
      m_vpLocks[1][i]->SetIsoValueRelative(isoval, false);
    }
  }
}

void RenderWindow::SetIsoValueRelative(float isoval, bool propagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setIsoValueRelative", isoval);
  if(propagate) {
    for(size_t i=0; i < m_vpLocks[1].size(); ++i) {
      m_vpLocks[1][i]->SetIsoValueRelative(isoval, false);
    }
  }
}

void RenderWindow::SetCVIsoValue(float fIsoVal, bool bPropagate) {
  // CV = clear view
  /// @todo Provenance: Need final iso value instead of continuous change.
  /// @todo Update cv isovalue slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVIsoValue", fIsoVal);
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVIsoValue(fIsoVal, false);
    }
  }
}

void RenderWindow::SetCVSize(float fSize, bool bPropagate) {
  /// @todo Provenance: Need final size value instead of continuous change.
  /// @todo Update slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVSize", fSize);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVSize(fSize, false);
    }
  }
}

void RenderWindow::SetCVContextScale(float fScale, bool bPropagate) {
  /// @todo Provenance: Need final scale value instead of continuous change.
  /// @todo Update slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVContextScale", fScale);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVContextScale(fScale, false);
    }
  }
}

void RenderWindow::SetCVBorderScale(float fScale, bool bPropagate) {
  /// @todo Provenance: Need final scale value instead of continuous change.
  /// @todo Update slider from hook.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVBorderScale", fScale);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVBorderScale(fScale, false);
    }
  }
}

void RenderWindow::SetGlobalBBox(bool bRenderBBox, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setGlobalBBox", bRenderBBox);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetGlobalBBox(bRenderBBox, false);
    }
  }
}

void RenderWindow::SetLocalBBox(bool bRenderBBox, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setLocalBBox", bRenderBBox);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetLocalBBox(bRenderBBox, false);
    }
  }
}
void RenderWindow::SetClipPlaneEnabled(bool enable, bool bPropagate)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->beginCommandGroup();
  if(enable) {
    ss->cexec(GetFirst3DRegion().fqName() + ".enableClipPlane", true);
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
    m_SavedClipLocked =
        ss->cexecRet<bool>(GetLuaAbstrRenderer().fqName()
                           + ".isClipPlaneLocked");
    ss->cexec(GetFirst3DRegion().fqName() + ".enableClipPlane", false);
    SetClipPlaneRelativeLock(true, bPropagate);
  }

  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneEnabled(enable, false);
    }
  }
  ss->endCommandGroup();
}

void RenderWindow::LuaCallbackEnableClipPlane(bool enable)
{
  // This function is called from Lua every time enableClipPlane is called.
  // We should be the active render window and we should change the checkbox
  // state.
  m_MainWindow->LuaCallbackToggleClipPlane(enable);
}

void RenderWindow::SetClipPlaneDisplayed(bool bDisp, bool bPropagate)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".showClipPlane", bDisp, GetFirst3DRegion());
  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneDisplayed(bDisp, false);
    }
  }
}

void RenderWindow::SetClipPlaneRelativeLock(bool bLock, bool bPropagate)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());

  // NOTE: Assuming first 3D render region.
  LuaClassInstance region = GetFirst3DRegion();
  RegionData* regionData = GetRegionData(GetFirst3DRegion());

  // Update clip plane.
  bool currentlyLocked = ss->cexecRet<bool>(GetLuaAbstrRenderer().fqName()
                                            + ".isClipPlaneLocked");
  if (currentlyLocked == true && bLock == false)
  {
    // (clip space ->) object space -> world space
    // Make clip plane relative to the world.
    FLOATMATRIX4 r = computeClipToVolToWorldTransform(region);
    regionData->toClipSpace = r.inverse();
  }
  else if (currentlyLocked == false && bLock == true)
  {
    // Obtain a new clipping space transform relative to the world space
    // transformation of the render region's volume.
    FLOATMATRIX4 vx = getHomogeneousVolToWorldTrafo(region);
    // Make clip plane relative to the object: O * C^-1 (clip plane was
    // relative to the world prior).
    FLOATMATRIX4 newClipSpace = vx * regionData->toClipSpace;
    regionData->toClipSpace = newClipSpace;
  }

  ss->cexec(GetLuaAbstrRenderer().fqName() + ".setClipPlaneLocked", bLock);

  // This should have NO effect on the current xform of the clipping plane.
  // Here as a visual debugging aid.
  updateClipPlaneTransform(region);

  if(bPropagate) {
    for(std::vector<RenderWindow*>::iterator locks = m_vpLocks[1].begin();
        locks != m_vpLocks[1].end(); ++locks) {
      (*locks)->SetClipPlaneRelativeLock(bLock, false);
    }
  }
}

void RenderWindow::SetIsosurfaceColor(const FLOATVECTOR3& vIsoColor, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setIsosurfaceColor", vIsoColor);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetIsosurfaceColor(vIsoColor, false);
    }
  }
}

void RenderWindow::SetCVColor(const FLOATVECTOR3& vIsoColor, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVColor", vIsoColor);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVColor(vIsoColor, false);
    }
  }
}

void RenderWindow::SetCV(bool bDoClearView, bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setClearViewEnabled", bDoClearView);
  if (bPropagate){
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCV(bDoClearView, false);
    }
  }
}

void RenderWindow::SetCVFocusPos(LuaClassInstance region,
                                 const INTVECTOR2& viMousePos,
                                 bool bPropagate) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setCVFocusPos", region, viMousePos);
  if (bPropagate) {
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetCVFocusPos(region, viMousePos, false);
    }
  }
}

void RenderWindow::SetTimestep(size_t t, bool propagate)
{
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  ss->cexec(rn + ".setTimestep", t);
  if(propagate) {
    for (size_t i = 0;i<m_vpLocks[1].size();i++) {
      m_vpLocks[1][i]->SetTimestep(t, false);
    }
  }
}

void RenderWindow::SetLogoParams(QString strLogoFilename, int iLogoPos) {
  m_MasterController.LuaScript()->cexec(
      GetLuaAbstrRenderer().fqName() + ".setLogoParams",
      strLogoFilename.toStdWString(),iLogoPos);
}

void RenderWindow::SetAbsoluteViewLock(bool bAbsoluteViewLock) {
  m_bAbsoluteViewLock = bAbsoluteViewLock;
}

pair<double,double> RenderWindow::GetDynamicRange() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  LuaClassInstance dataset = GetRendererDataset();
  pair<double,double> range = ss->cexecRet<pair<double,double> >(
      dataset.fqName() + ".getRange");

  // Old UVFs lack a maxmin data block, && will say the min > the max.
  if (range.first > range.second) {
    LuaClassInstance tf1d = GetRendererTransferFunction1D();
    return make_pair(0,
                     double(ss->cexecRet<size_t>(tf1d.fqName() + ".getSize")));
  }
  else {
    return range;
  }
}

FLOATVECTOR3 RenderWindow::GetIsosurfaceColor() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<FLOATVECTOR3>(rn + ".getIsosurfaceColor");
}

FLOATVECTOR3 RenderWindow::GetCVColor() const {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();
  return ss->cexecRet<FLOATVECTOR3>(rn + ".getCVColor");
}

void RenderWindow::ResizeRenderer(int width, int height)
{
  int h = GetQtWidget()->size().height();
  m_fHighDPIScale = float(h)/height;
  m_vWinDim = UINTVECTOR2((unsigned int)width, (unsigned int)height);

  /// @fixme Create a setMaxCoord function for the region.
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();

  LuaClassInstance firstRenRegion = GetActiveRenderRegions()[0];
  RenderRegion* regPtr = firstRenRegion.getRawPointer<RenderRegion>(ss);

  if (m_LuaAbstrRenderer.isValid(ss) && m_bRenderSubsysOK) {
    switch (GetViewMode()) {
      case VM_SINGLE :
        regPtr->maxCoord = m_vWinDim;
        break;
      case VM_TWOBYTWO :
        UpdateWindowFraction();
        break;
      default: break; //nothing to do...
    };

    ss->cexec(rn + ".resize", UINTVECTOR2(width, height));
    SetupArcBall();
    std::ostringstream wsize;
    wsize << m_vWinDim[0] << " " << m_vWinDim[1] << std::ends;
  }
}

void RenderWindow::PaintRenderer()
{
  if (!m_strMultiRenderGuard.tryLock()) {
    MESSAGE("Rejecting dublicate Paint call");
    return;
  }

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  string rn = m_LuaAbstrRenderer.fqName();

  if (m_LuaAbstrRenderer.isValid(ss) && m_bRenderSubsysOK) {
    if (!ss->cexecRet<bool>(rn + ".paint")) {
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
      T_ERROR("AbstrRenderer::Paint() call failed.");
    }

    if (GetQtWidget()->isActiveWindow()) {
      unsigned int iLevelCount = GetCurrentSubFrameCount();
      unsigned int iWorkingLevelCount = GetWorkingSubFrame();
      unsigned int iBrickCount = GetCurrentBrickCount();
      unsigned int iWorkingBrick = GetWorkingBrick();
      unsigned int iMinLODIndex = GetMinLODIndex();

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

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  for (int i=0; i < MAX_RENDER_REGIONS; ++i) {
    for (int j=0; j < NUM_WINDOW_MODES; ++j) {
      LuaClassInstance region = luaRenderRegions[i][j];
      GetRegionData(region)->toClipSpace = mIdentity;

      ss->cexec(region.fqName() + ".setRotation4x4", mIdentity);
      ss->cexec(region.fqName() + ".setTranslation4x4", mIdentity);

      SetClipPlane(region, ExtendedPlane());
    }
  }

  SetWindowFraction2x2(FLOATVECTOR2(0.5f, 0.5f));

  string an = GetLuaAbstrRenderer().fqName();
  ss->cexec(an + ".resetUpDir");
  ss->cexec(an + ".resetViewDir");
  ss->cexec(an + ".resetViewPos");
  ss->cexec(an + ".transfer3DRotationToMIP");

  m_fFirstPersonSpeed = s_fFirstPersonSpeed;
}


void RenderWindow::SetCurrent1DHistScale(const float value) {
  m_1DHistScale = value;
}

void RenderWindow::SetCurrent2DHistScale(const float value) {
  m_2DHistScale = value;
}

float RenderWindow::GetCurrent1DHistScale() const {
  return m_1DHistScale;
}

float RenderWindow::GetCurrent2DHistScale() const {
  return m_2DHistScale;
}

LuaClassInstance RenderWindow::GetLuaAbstrRenderer() const {
  return m_LuaAbstrRenderer;
}

LuaClassInstance RenderWindow::GetLuaInstance() const {
  return m_LuaThisClass;
}

void RenderWindow::SetProvTransformAndClip(tuvok::LuaClassInstance region,
                                           FLOATMATRIX4 m){
  // Extract the translational component from the matrix.
  FLOATMATRIX4 t4;
  t4.m41 = m.m41;
  t4.m42 = m.m42;
  t4.m43 = m.m43;

  FLOATMATRIX4 r4;
  r4.m11 = m.m11; r4.m12 = m.m12; r4.m13 = m.m13;
  r4.m21 = m.m21; r4.m22 = m.m22; r4.m23 = m.m23;
  r4.m31 = m.m31; r4.m32 = m.m32; r4.m33 = m.m33;

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(region.fqName() + ".setRotation4x4", r4);
  ss->cexec(region.fqName() + ".setTranslation4x4", t4);
  ss->cexec(region.fqName() + ".setClipPlane", m_ClipPlane);
}

void RenderWindow::SetProvRotationAndClip(tuvok::LuaClassInstance region,
                                          FLOATMATRIX4 r) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(region.fqName() + ".setRotation4x4", r);
  ss->cexec(region.fqName() + ".setClipPlane", m_ClipPlane);
}

void RenderWindow::SetProvTransAndClip(tuvok::LuaClassInstance region,
                                       FLOATMATRIX4 t) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(region.fqName() + ".setTranslation4x4", t);
  ss->cexec(region.fqName() + ".setClipPlane", m_ClipPlane);
}

void RenderWindow::LuaSetTransform(FLOATMATRIX4 m) {
  SetProvTransformAndClip(GetFirst3DRegion(), m);
}

void RenderWindow::LuaSetRotation(FLOATMATRIX3 m) {
  FLOATMATRIX4 r4;
  r4.m11 = m.m11; r4.m12 = m.m12; r4.m13 = m.m13;
  r4.m21 = m.m21; r4.m22 = m.m22; r4.m23 = m.m23;
  r4.m31 = m.m31; r4.m32 = m.m32; r4.m33 = m.m33;
  SetProvRotationAndClip(GetFirst3DRegion(), r4);
}
void RenderWindow::LuaSetTranslation(VECTOR3<float> v) {
  FLOATMATRIX4 t4;
  t4.m41 = v.x;
  t4.m42 = v.y;
  t4.m43 = v.z;
  SetProvTransAndClip(GetFirst3DRegion(), t4);
}

void RenderWindow::LuaSetTranslationAs4x4(FLOATMATRIX4 m) {
  SetProvTransAndClip(GetFirst3DRegion(), m);
}

void RenderWindow::LuaSetRotationAs4x4(FLOATMATRIX4 m) {
  SetProvRotationAndClip(GetFirst3DRegion(), m);
}

void RenderWindow::LuaResizeWindow(const UINTVECTOR2& newSize) {
  UINTVECTOR2 renderSize = GetRendererSize();
  UINTVECTOR2 windowSize(GetQtWidget()->size().width(), 
                         GetQtWidget()->size().height());

  UINTVECTOR2 winDecoSize = windowSize-renderSize;
  QMdiSubWindow* w = dynamic_cast<QMdiSubWindow*>(GetQtWidget()->parent());
  if(w) {
    w->resize(newSize.x+winDecoSize.x, newSize.y+winDecoSize.y);
  }
}

void RenderWindow::LuaSetLighting(bool enabled) {
  this->SetUseLighting(enabled);
}

void RenderWindow::LuaLoad1DTFqn(const std::wstring& tf) {
  this->m_MainWindow->LoadTransferFunction1D(tf);
}

void RenderWindow::LuaLoad2DTFqn(const std::wstring& tf) {
  this->m_MainWindow->LoadTransferFunction2D(tf);
}

void RenderWindow::LuaSetClipPlane(const FLOATMATRIX4& m) {
  LuaClassInstance rgn = this->GetFirst3DRegion();
  ExtendedPlane p;
  p.Transform(m, false);
  SetClipPlane(rgn, p);
}

void RenderWindow::RegisterLuaFunctions(
    LuaClassRegistration<RenderWindow>& reg, RenderWindow* me,
    LuaScripting* ss) {
  ss->vPrint("Registering render window functions.");

  me->m_LuaThisClass = reg.getLuaInstance();

  string id;

  LuaClassInstance ar = me->GetLuaAbstrRenderer();

  // Hooks for updating the UI when events in Tuvok occur.
  me->m_MemReg.strictHook(me, &RenderWindow::LuaCallbackEnableClipPlane,
                          me->GetFirst3DRegion().fqName() + ".enableClipPlane");

  // Inherit functions from the Abstract Renderer.
  reg.inherit(ar, "getDataset");
  reg.inherit(ar, "setBGColors");
  reg.inherit(ar, "setTextColor");
  reg.inherit(ar, "setBlendPrecision");
  reg.inherit(ar, "setLogoParams");
  reg.inherit(ar, "setRendererTarget");

  // Register our own functions.
  id = reg.function(&RenderWindow::GetLuaAbstrRenderer, "getRawRenderer",
                    "Returns the Tuvok abstract renderer instance.",
                    false);
  ss->addReturnInfo(id, "Lua class instance of Tuvok's abstract renderer."
      "Generally, you should use the methods exposed by the render window "
      "instead of resorting to raw access to the renderer.");

  id = reg.function(&RenderWindow::SetPerfMeasures, "setPerformanceMeasures",
                    "Sets various performance measures. See info for a detailed"
                    " description of the parameters.", true);
  ss->addParamInfo(id, 0, "minFramerate", "Minimum framerate.");
  ss->addParamInfo(id, 1, "lowResRender", "If true, render low res intermediate"
      "results.");
  ss->addParamInfo(id, 2, "screenResDecFactor", "");  // screen res decrease?
  ss->addParamInfo(id, 3, "sampleDecFactor", "");     // sample rate decrease?
  ss->addParamInfo(id, 4, "LODDelay", "LOD Delay.");
  ss->addParamInfo(id, 5, "activeTS", "");
  ss->addParamInfo(id, 6, "inactiveTS", "");

  id = reg.function(&RenderWindow::CaptureFrame, "screenCapture",
                    "Screenshot of the current volume.", false);
  ss->addParamInfo(id, 0, "filename", "Filename of the screen cap.");
  ss->addParamInfo(id, 1, "preserveTransparency", "True if you want to preserve"
      " transparency in the screen cap.");

  id = reg.function(&RenderWindow::LuaSetTransform, "setTransform",
                    "Sets the 4x4 matrix transformation for the first 3D "
                    "render region.", true);
  id = reg.function(&RenderWindow::LuaSetRotation, "setRotation",
                    "Sets 3x3 rotation matrix for the first 3D "
                    "render region.", true);
  id = reg.function(&RenderWindow::LuaSetTranslation, "setTranslation",
                    "Sets translation for the first 3D "
                    "render region.", true);
  id = reg.function(&RenderWindow::LuaSetRotationAs4x4, "setRotationAs4x4",
                    "Sets 3x3 rotation matrix for the first 3D "
                    "render region.", true);
  id = reg.function(&RenderWindow::LuaSetTranslationAs4x4,"setTranslationAs4x4",
                    "Sets translation for the first 3D "
                    "render region.", true);
  id = reg.function(&RenderWindow::LuaResizeWindow,"resize",
                    "Resize this render window.", true);
  id = reg.function(&RenderWindow::LuaSetLighting, "lighting",
                    "turn lighting on and off", true);
  id = reg.function(&RenderWindow::LuaLoad1DTFqn, "tfqn1d",
                    "load a new (1D) transfer function", true);
  id = reg.function(&RenderWindow::LuaLoad2DTFqn, "tfqn2d",
                    "load a new (2D) transfer function", true);
  reg.function(&RenderWindow::UpdateWindow, "paint", "forces paint", false);

  reg.function(&RenderWindow::RotateViewer, "rotateViewer",
               "Rotates the viewer in x and y viewing direction", true);
  reg.function(&RenderWindow::MoveViewer, "moveViewer",
               "Moves the viewer in the viewing coordinate frame", true);

  reg.function(&RenderWindow::SetFirstPersonMode, "setFirstPersonMode",
               "Enables/disables the first person mode", true);
  reg.function(&RenderWindow::GetFirstPersonMode, "getFirstPersonMode",
               "Returns true if the first person mode is enabled", true);
  reg.function(&RenderWindow::LuaSetClipPlane, "setClipPlane",
               "resets the clipping plane to the given 4x4 transform", true);
  id = reg.function(&RenderWindow::CaptureSubframe, "captureSubframe",
                    "captures whatever's in the buffer now", true);
  ss->addParamInfo(id, 0, "filename", "Filename to save it in");
  id = reg.function(&RenderWindow::ClipDelta, "clipDelta",
                    "moves the clip plane in the direction of its normal",
                    true);
  ss->addParamInfo(id, 0, "floating-point", "amount to move the plane [0--1]");
}

void RenderWindow::ClipDelta(float d)
{
  if(d < -1.0 || d > 1.0) {
    WARNING("large clip plane movement.. scripting bug?  Proceeding anyway.");
  }
  LuaClassInstance rr3d = this->GetFirst3DRegion();
  SetClipTranslationDelta(rr3d, FLOATVECTOR3(d,d,0), false, false);
}
