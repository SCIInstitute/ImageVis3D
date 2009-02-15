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
#include <StdDefines.h>
#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/Basics/ArcBall.h"
#include "../Tuvok/Basics/Plane.h"

class MainWindow;

class RenderWindow
{
  public:
    RenderWindow(MasterController& masterController,
                 MasterController::EVolumeRendererType eType,
                 const QString& dataset,
                 unsigned int iCounter,
                 QWidget* parent,
                 const UINTVECTOR2& vMinSize = UINTVECTOR2(50, 50),
                 const UINTVECTOR2& vDefaultSize= UINTVECTOR2(400, 400));

    virtual ~RenderWindow();

    QString GetDatasetName() const {return m_strDataset;}
    QString GetWindowID() const {return m_strID;}
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    AbstrRenderer* GetRenderer() {return m_Renderer;}
    void CheckForRedraw();
    void SetRendermode(AbstrRenderer::ERenderMode eRenderMode, bool bPropagate=true);
    AbstrRenderer::ERenderMode GetRendermode() const {
        return m_Renderer->GetRendermode();
    }
    void SetColors(FLOATVECTOR3 vBackColors[2], FLOATVECTOR4 vTextColor);
    virtual void SetBlendPrecision(AbstrRenderer::EBlendPrecision eBlendPrecisionMode);
    void SetPerfMeasures(unsigned int iMinFramerate, unsigned int iLODDelay, unsigned int iActiveTS, unsigned int iInactiveTS);
    bool CaptureFrame(const std::string& strFilename, bool bPreserveTransparency);
    bool CaptureSequenceFrame(const std::string& strFilename, bool bPreserveTransparency, std::string* strRealFilename=NULL);
    bool CaptureMIPFrame(const std::string& strFilename, float fAngle, bool bOrtho, bool bFinalFrame, bool bUseLOD, bool bPreserveTransparency, std::string* strRealFilename=NULL);
    void ToggleHQCaptureMode();
    void Translate(const FLOATMATRIX4& mTranslation) {SetTranslation(mTranslation*m_mAccumulatedTranslation);}
    void Rotate(const FLOATMATRIX4& mRotation) {SetRotation(mRotation*m_mAccumulatedRotation,mRotation*m_mAccumulatedRotation);}
    void SetCaptureRotationAngle(float fAngle);
    bool IsRenderSubsysOK() const {return m_bRenderSubsysOK;}

    static const size_t               ms_iLockCount = 4;
    std::vector<RenderWindow*>        m_vpLocks[ms_iLockCount];

    void SetLogoParams(QString strLogoFilename, int iLogoPos);

    void SetTranslationDelta(const FLOATVECTOR3& trans, bool bPropagate);
    void SetRotationDelta(const FLOATMATRIX4& rotDelta, bool bPropagate);
    void SetClipPlane(const ExtendedPlane &p);
    void SetClipTranslationDelta(const FLOATVECTOR3& trans, bool);
    void SetClipRotationDelta(const FLOATMATRIX4& rotDelta, bool);
    void CloneViewState(RenderWindow* other);
    void FinalizeRotation(bool bPropagate);
    void CloneRendermode(RenderWindow* other);
    void SetAbsoluteViewLock(bool bAbsoluteViewLock);

    void SetAvoidCompositing(bool bAvoidCompositing);
    bool GetAvoidCompositing() const;

    void SetUseLighting(bool bLighting, bool bPropagate=true);
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
    void SetCVFocusPos(const FLOATVECTOR2& vMousePos, bool bPropagate=true);

    size_t GetDynamicRange() const;
    FLOATVECTOR3 GetIsosufaceColor() const;
    FLOATVECTOR3 GetCVColor() const;

    static const std::string& GetVendorString() {return ms_gpuVendorString;}
    static UINT32 GetMax3DTexDims() {return ms_iMax3DTexDims;}

    virtual QWidget* GetQtWidget() = 0;

    virtual void ToggleFullscreen() = 0;

  public: // public slots:
    virtual void ToggleRenderWindowView2x2();
    virtual void ToggleRenderWindowViewSingle();
    virtual void SetTimeSlices(unsigned int iActive, unsigned int iInactive) {m_iTimeSliceMSecsActive = iActive; m_iTimeSliceMSecsInActive = iInactive;}

  protected:
    QString           m_strDataset;
    QString           m_strID;
    AbstrRenderer*    m_Renderer;
    MasterController& m_MasterController;
    bool              m_bRenderSubsysOK;
    UINTVECTOR2       m_vWinDim;
    UINTVECTOR2       m_vMinSize;
    UINTVECTOR2       m_vDefaultSize;

    static std::string ms_gpuVendorString;
    static UINT32      ms_iMax3DTexDims;

    void SetupArcBall();

    void ResizeRenderer(int width, int height);
    void PaintRenderer();
    virtual void InitializeRenderer() = 0;

    // Qt widget connector calls
    virtual void UpdateWindow() = 0;
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

  private:
    /// Called when the mouse is moved, but in a mode where the clip plane
    /// should be manipulated instead of the dataset.
    /// @param pos       new position of the mouse cursor
    /// @param rotate    if this should rotate the clip plane
    /// @param translate if this should translate the clip plane
    bool MouseMoveClip(INTVECTOR2 pos, bool rotate, bool translate);

    /// Called for a mouse update when in the 3D view mode.
    /// @param pos new position of the mouse cursor
    /// @param clearview if this action should affect clearview
    /// @param rotate    should this action rotate the data
    /// @param translate should this action translate the data
    bool MouseMove3D(INTVECTOR2 pos, bool clearview, bool rotate,
                     bool translate);

    void SetRotation(const FLOATMATRIX4& mAccumulatedRotation,
                     const FLOATMATRIX4& mCurrentRotation);
    void SetTranslation(const FLOATMATRIX4& mAccumulatedTranslation);

    void ResetRenderingParameters();

  private:
    MasterController::EVolumeRendererType m_eRendererType;
    MainWindow*       m_MainWindow;
    unsigned int      m_iTimeSliceMSecsActive;
    unsigned int      m_iTimeSliceMSecsInActive;

    ArcBall           m_ArcBall;
    INTVECTOR2        m_viRightClickPos;
    INTVECTOR2        m_viMousePos;
    FLOATMATRIX4      m_mCurrentRotation;
    FLOATMATRIX4      m_mAccumulatedRotation;
    FLOATMATRIX4      m_mCaptureStartRotation;
    FLOATMATRIX4      m_mAccumulatedTranslation;
    bool              m_bAbsoluteViewLock;
    bool              m_bCaptureMode;

    ArcBall           m_ClipArcBall;
    FLOATMATRIX4      m_mCurrentClipRotation;
    FLOATMATRIX4      m_mAccumulatedClipRotation;
    FLOATMATRIX4      m_mAccumulatedClipTranslation;
    ExtendedPlane     m_ClipPlane;
};

#endif // RENDERWINDOW_H
