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


//!    File   : RenderWindow.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <string>
#include <QtGui/QListWidget>
#include <QtCore/QMutex>
#include <StdDefines.h>
#include "../Tuvok/Basics/ArcBall.h"
#include "../Tuvok/Basics/Plane.h"
#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/Renderer/AbstrRenderer.h"
#include "../Tuvok/LUAScripting/LUAScripting.h"
#include "../Tuvok/LUAScripting/LUAClassRegistration.h"

#ifdef DETECTED_OS_WINDOWS
# include <unordered_map>
#else
# include <tr1/unordered_map>
#endif

class MainWindow;

class RenderWindow
{
  public:
    RenderWindow(tuvok::MasterController& masterController,
                 tuvok::MasterController::EVolumeRendererType eType,
                 const QString& dataset,
                 unsigned int iCounter,
                 QWidget* parent,
                 const UINTVECTOR2& vMinSize = UINTVECTOR2(50, 50),
                 const UINTVECTOR2& vDefaultSize= UINTVECTOR2(512, 512));

    virtual ~RenderWindow();

    QString GetDatasetName() const {return m_strDataset;}
    QString GetWindowID() const {return m_strID;}
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    tuvok::AbstrRenderer* GetRenderer() {return m_Renderer;}
    void CheckForRedraw();
    void SetRendermode(tuvok::AbstrRenderer::ERenderMode eRenderMode,
                       bool bPropagate=true);
    tuvok::AbstrRenderer::ERenderMode GetRenderMode() const;

    void SetColors(FLOATVECTOR3 vTopColor, FLOATVECTOR3 vBotColor,
                   FLOATVECTOR4 vTextColor);
    virtual void SetBlendPrecision(tuvok::AbstrRenderer::EBlendPrecision
                                     eBlendPrecisionMode);
    void SetPerfMeasures(unsigned int iMinFramerate,
                         bool bRenderLowResIntermediateResults,
                         float fScreenResDecFactor, float fSampleDecFactor,
                         unsigned int iLODDelay,
                         unsigned int iActiveTS, unsigned int iInactiveTS);
    bool CaptureFrame(const std::string& strFilename,
                      bool bPreserveTransparency);
    bool CaptureSequenceFrame(const std::string& strFilename,
                              bool bPreserveTransparency,
                              std::string* strRealFilename=NULL);
    bool CaptureMIPFrame(const std::string& strFilename,
                         float fAngle, bool bOrtho, bool bFinalFrame,
                         bool bUseLOD, bool bPreserveTransparency,
                         std::string* strRealFilename=NULL);
    void ToggleHQCaptureMode();
    void Translate(const FLOATMATRIX4& mTranslation,
                   tuvok::RenderRegion *region=NULL);
    void Rotate(const FLOATMATRIX4& mRotation,
                tuvok::RenderRegion *region=NULL);
    void SetCaptureRotationAngle(float fAngle);
    bool IsRenderSubsysOK() const {return m_bRenderSubsysOK;}

    static const size_t               ms_iLockCount = 4;
    std::vector<RenderWindow*>        m_vpLocks[ms_iLockCount];

    void SetLogoParams(QString strLogoFilename, int iLogoPos);

    void SetTranslationDelta(tuvok::RenderRegion *region,
                             const FLOATVECTOR3& trans,
                             bool bPropagate);
    void SetRotationDelta(tuvok::RenderRegion *region,
                          const FLOATMATRIX4& rotDelta,
                          bool bPropagate);
    void SetClipPlane(tuvok::RenderRegion *region, const ExtendedPlane &p);
    void SetClipTranslationDelta(tuvok::RenderRegion *region,
                                 const FLOATVECTOR3& trans,
                                 bool, bool);
    void SetClipRotationDelta(tuvok::RenderRegion *region,
                              const FLOATMATRIX4& rotDelta,
                              bool, bool);
    void SetPlaneAtClick(const ExtendedPlane& plane, bool propagate=true);
    void CloneViewState(RenderWindow* other);
    void FinalizeRotation(const tuvok::RenderRegion *region, bool bPropagate);
    void CloneRendermode(RenderWindow* other);
    void SetAbsoluteViewLock(bool bAbsoluteViewLock);

    void SetInvMouseWheel(const bool bInvWheel) {m_bInvWheel = bInvWheel;}
    bool GetInvMouseWheel() const {return m_bInvWheel;}

    void SetCurrent1DHistScale(const float value);
    void SetCurrent2DHistScale(const float value);

    float GetCurrent1DHistScale() const;
    float GetCurrent2DHistScale() const;

    void SetUseLighting(bool bLighting, bool bPropagate=true);
    bool GetUseLighting() const;
    void SetSampleRateModifier(float fSampleRateModifier, bool bPropagate=true);
    void SetIsoValue(float fIsoVal, bool bPropagate=true);
    void SetCVIsoValue(float fIsoVal, bool bPropagate=true);
    void SetCVSize(float fSize, bool bPropagate=true);
    void SetCVContextScale(float fScale, bool bPropagate=true);
    void SetCVBorderScale(float fScale, bool bPropagate=true);
    void SetGlobalBBox(bool bRenderBBox, bool bPropagate=true);
    void SetLocalBBox(bool bRenderBBox, bool bPropagate=true);
    void SetClipPlaneEnabled(bool, bool bPropagate = true);
    void SetClipPlaneDisplayed(bool, bool bPropagate = true);
    void SetClipPlaneRelativeLock(bool, bool bPropagate = true);
    void SetIsosufaceColor(const FLOATVECTOR3& vIsoColor, bool bPropagate=true);
    void SetCVColor(const FLOATVECTOR3& vIsoColor, bool bPropagate=true);
    void SetCV(bool bDoClearView, bool bPropagate=true);
    void SetCVFocusPos(tuvok::RenderRegion *region, const INTVECTOR2& vMousePos,
                       bool bPropagate=true);
    void SetTimestep(size_t, bool=true);

    // @return the range of the currently loaded dataset
    std::pair<double,double> GetDynamicRange() const;
    FLOATVECTOR3 GetIsosufaceColor() const;
    FLOATVECTOR3 GetCVColor() const;

    static const std::string& GetVendorString() {return ms_gpuVendorString;}
    static uint32_t GetMax3DTexDims() {return ms_iMaxVolumeDims;}
    static bool Get3DTexInDriver() {return ms_b3DTexInDriver;}
    static bool Get3ImageLoadStoreInDriver() {
      return ms_bImageLoadStoreInDriver;
    }

    virtual QWidget* GetQtWidget() = 0;

    virtual void ToggleFullscreen() = 0;
    virtual void UpdateWindow() = 0;
    /// Ensures context has been created, without performing a render.
    virtual void InitializeContext() = 0;

    tuvok::LuaClassInstance GetLuaInstance() const;

    tuvok::RenderRegion* GetRegionUnderCursor(INTVECTOR2 vPos) const;

    enum EViewMode {
      VM_SINGLE = 0,  /**< a single large image */
      VM_TWOBYTWO,    /**< four small images */
      VM_INVALID
    };
    EViewMode GetViewMode() const {return m_eViewMode;}

    enum RegionSplitter {
      REGION_SPLITTER_HORIZONTAL_2x2,
      REGION_SPLITTER_VERTICAL_2x2,
      REGION_SPLITTER_BOTH_2x2,
      REGION_SPLITTER_NONE
    };

    RegionSplitter GetRegionSplitter(INTVECTOR2 pos) const;

    tuvok::RenderRegion3D* GetFirst3DRegion();
    const std::vector<tuvok::RenderRegion*>& GetActiveRenderRegions() const;
    void SetActiveRenderRegions(const std::vector<tuvok::RenderRegion*>&) const;
    void ResetRenderingParameters();

    static void RegisterLuaFunctions(
          tuvok::LuaClassRegistration<RenderWindow>& reg,
          RenderWindow* me,
          tuvok::LuaScripting* ss);

  public: // public slots:
    virtual void ToggleRenderWindowView2x2();
    virtual void ToggleRenderWindowViewSingle();
    virtual bool SetRenderWindowView3D();
    virtual void SetTimeSlices(unsigned int iActive, unsigned int iInactive)
    {m_iTimeSliceMSecsActive = iActive; m_iTimeSliceMSecsInActive = iInactive;}

  protected:
    QMutex                    m_strMultiRenderGuard;
    QString                   m_strDataset;
    QString                   m_strID;
    tuvok::AbstrRenderer*     m_Renderer;
    tuvok::MasterController&  m_MasterController;
    bool                      m_bRenderSubsysOK;
    RegionSplitter            selectedRegionSplitter;
    UINTVECTOR2               m_vWinDim;
    UINTVECTOR2               m_vMinSize;
    UINTVECTOR2               m_vDefaultSize;
    EViewMode                 m_eViewMode;
    FLOATVECTOR2              m_vWinFraction;
    static const int          regionSplitterWidth = 6;
    tuvok::RenderRegion*      activeRegion; // The region that should have focus
    tuvok::LuaClassInstance   m_ThisClass;
    tuvok::LuaClassInstance   m_LuaRenderer;
    MainWindow*               m_MainWindow;

    // If later on more than 4 regions are desired this can either be
    // increased or something fancier than an array can be used.
    static const int MAX_RENDER_REGIONS = 4;
    static const int NUM_WINDOW_MODES = 4;
    tuvok::RenderRegion *renderRegions[MAX_RENDER_REGIONS][NUM_WINDOW_MODES];
    int selected2x2Regions[4]; //index into renderRegions

    static std::string ms_gpuVendorString;
    static uint32_t      ms_iMaxVolumeDims;
    static bool        ms_b3DTexInDriver;
    static bool        ms_bImageLoadStoreInDriver;

    struct RegionData {
      ArcBall arcBall;
      ArcBall clipArcBall;

      FLOATMATRIX4 clipRotation[2];
    };

    RegionData regionDatas[MAX_RENDER_REGIONS][NUM_WINDOW_MODES];

    typedef std::tr1::unordered_map<const tuvok::RenderRegion*, RegionData*>
      RegionDataMap;
    RegionDataMap regionDataMap;

    RegionData* GetRegionData(const tuvok::RenderRegion* const) const;

    void SetupArcBall();

    void ResizeRenderer(int width, int height);
    void PaintRenderer();
    virtual void PaintOverlays() = 0;
    virtual void RenderSeparatingLines() = 0;
    virtual void InitializeRenderer() = 0;

    // Qt widget connector calls
    virtual void ForceRepaint() = 0;
    virtual void EmitStereoDisabled() = 0;
    virtual void EmitRenderWindowViewChanged(int iViewID) = 0;
    virtual void EmitWindowActive() = 0;
    virtual void EmitWindowInActive() = 0;
    virtual void EmitWindowClosing() = 0;

    void MousePressEvent(QMouseEvent *event);
    void MouseReleaseEvent(QMouseEvent *event);
    void MouseMoveEvent(QMouseEvent *event);
    void WheelEvent(QWheelEvent *event);
    void CloseEvent(QCloseEvent *event);
    void FocusInEvent(QFocusEvent * event);
    void FocusOutEvent ( QFocusEvent * event );
    void KeyPressEvent ( QKeyEvent * event );
    void Cleanup();
    void Initialize();
    virtual void SwapBuffers() {}

    void UpdateCursor(const tuvok::RenderRegion *region,
                      INTVECTOR2 pos, bool translate);

    void BaseSetLuaDefaults();

    /// @param[in,out] newRenderRegions with coordinates updated to reflect the
    /// new view mode.
    /// @param[in] eViewMode The new ViewMode to use.
    virtual void SetViewMode(const std::vector<tuvok::RenderRegion*> &newRenderRegions,
                             EViewMode eViewMode);

    void SetWindowFraction2x2(FLOATVECTOR2 f);
    FLOATVECTOR2 WindowFraction2x2() const { return m_vWinFraction; }
    void UpdateWindowFraction();

    tuvok::RenderRegion* GetCorrespondingRenderRegion(
        const RenderWindow* otherRW,
        const tuvok::RenderRegion* myRR) const;
    tuvok::MasterController::EVolumeRendererType m_eRendererType;

  private:
    /// Called when the mouse is moved, but in a mode where the clip plane
    /// should be manipulated instead of the dataset.
    /// @param pos       new position of the mouse cursor
    /// @param rotate    if this should rotate the clip plane
    /// @param translate if this should translate the clip plane
    /// @param region    The active RenderRegion the mouse/user is operating in.
    bool MouseMoveClip(INTVECTOR2 pos, bool rotate, bool translate,
                       tuvok::RenderRegion *region);

    /// Called for a mouse update when in the 3D view mode.
    /// @param pos new position of the mouse cursor
    /// @param clearview if this action should affect clearview
    /// @param rotate    should this action rotate the data
    /// @param translate should this action translate the data
    /// @param region    The active RenderRegion the mouse/user is operating in.
    bool MouseMove3D(INTVECTOR2 pos, bool clearview, bool rotate, bool translate,
                     tuvok::RenderRegion *region);

    void SetRotation(tuvok::RenderRegion *region, const FLOATMATRIX4& newRotation);
    void SetTranslation(tuvok::RenderRegion *region,
                        const FLOATMATRIX4& mAccumulatedTranslation);

    /// Returns the Lua Renderer. This is used instead of inheriting the methods
    /// from the renderer, because, for the most part, the user won't care about
    /// the methods in the renderer().
    tuvok::LuaClassInstance GetLuaRenderer() const;

  private:

    unsigned int      m_iTimeSliceMSecsActive;
    unsigned int      m_iTimeSliceMSecsInActive;
    float             m_1DHistScale;
    float             m_2DHistScale;

    INTVECTOR2        initialClickPos;
    INTVECTOR2        m_viMousePos;
    FLOATMATRIX4      m_mCaptureStartRotation;
    bool              m_bAbsoluteViewLock;
    bool              m_bInvWheel;
    tuvok::AbstrRenderer::ERendererTarget m_RTModeBeforeCapture;

    FLOATMATRIX4      m_mAccumulatedClipTranslation;
    ExtendedPlane     m_ClipPlane;
    ExtendedPlane     m_PlaneAtClick; ///< temp storage to maintain the clip
                                      /// plane state when the user
                                      /// left-clicked.
    /// We implicitly lock the clip plane to the dataset when  it is disabled.
    /// This saves the value at disabling time, so we can restore it when the
    /// clip plane is re-enabled.
    bool              m_SavedClipLocked;
};

#endif // RENDERWINDOW_H
