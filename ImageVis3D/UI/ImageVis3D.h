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


//!    File   : ImageVis3D.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include <StdDefines.h>
#include <string>
#include <vector>

class ExtendedPlane;
class TextfileOut;

#include <ui_ImageVis3D.h>
#include "RenderWindowGL.h"
#if defined(_WIN32) && defined(USE_DIRECTX)
#include "RenderWindowDX.h"
#endif

#include "QLightPreview.h"
#include "Q1DTransferFunction.h"
#include "Q2DTransferFunction.h"
#include "DebugOut/QTOut.h"
#include <UI/SettingsDlg.h>
#include <UI/Welcome.h>
#include <UI/MetadataDlg.h>
#include "DebugScriptWindow.h"
#include <QtNetwork/QNetworkAccessManager>

#include "../Tuvok/LuaScripting/LuaScripting.h"
#include "../Tuvok/LuaScripting/LuaClassRegistration.h"
#include "../Tuvok/LuaScripting/LuaMemberReg.h"

class QDragEnterEvent;
class QDropEvent;
class QNetworkReply;
class QFile;
class QTimer;
class FTPDialog;
class PleaseWaitDialog;
class ScaleAndBiasDlg;

static const int IV3D_TIMER_INTERVAL = 20; // in milliseconds

class MainWindow : public QMainWindow, protected Ui_MainWindow
{
  Q_OBJECT

  public:
    MainWindow(MasterController& masterController,
               bool bScriptMode = false,   // suppress dialog boxes
               QWidget* parent = 0,
               Qt::WindowFlags flags = 0);

    virtual ~MainWindow();

    QTOut* GetDebugOut() {return m_pDebugOut;}

    bool StayOpen() const {return m_bStayOpenAfterScriptEnd;}
    /// Starts the internal timer, used for checking if we should continue
    /// rendering.
    void StartTimer();
    const std::wstring& GetTempDir() {return m_strTempDir;}

    // Lua function binding

    /// Member function constructor for RenderWindowGL.
    RenderWindow* LuaCreateNewWindow(std::wstring dataset);

    void LuaResizeActiveWindow(const UINTVECTOR2& newSize);

    void LuaPlaceActiveWindow(const UINTVECTOR2& position);

    void closeMDISubWindowWithWidget(QWidget* widget);

    // Lua callback functions (called from Lua when actions are undone/redone).
    void LuaCallbackToggleClipPlane(bool bClip);

    // directly loads the TFqn; used for Lua scripting.
    void LoadTransferFunction1D(const std::wstring& tf);
    void LoadTransferFunction2D(const std::wstring& tf);

    bool RunLuaScript(const std::wstring& strFilename);

  public slots:
    void SetRenderProgressAnUpdateInfo(unsigned int iLODCount,
                                       unsigned int iCurrentCount,
                                       unsigned int iBrickCount,
                                       unsigned int iWorkingBrick,
                                       unsigned int iMinLODIndex,
                                       RenderWindow* pRenderWin);

  protected slots:
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

    void TransferToI3M();

    void FtpFail();
    void FtpSuccess();

    void SetCaptureFilename();
    void CaptureFrame();
    void CaptureSequence();
    void CaptureRotation();
    void LoadDataset();
    void LoadDataset(const std::wstring& strFilename);
    void LoadDirectory();
    void AddGeometry();
    void ExportGeometry();
    void RemoveGeometry();
    void CloseCurrentView();
    void ResizeCurrentView(int iSizeX, int iSizeY);
    void PlaceCurrentView(int iPosX, int iPosY);
    void CloneCurrentView();
    void CheckForUpdates();
    void OnlineHelp();
    void OpenManual();
    void OnlineVideoTut();
    void GetExampleData();
    void CloseWelcome();
    void ReportABug();

    void ToggleRenderWindowView2x2();
    void ToggleRenderWindowViewSingle();

    void Use1DTrans();
    void Use2DTrans();
    void UseIso();
    void DisableAllTrans();

    void Invert1DTransComp();
    void Transfer1DSetColors();
    void Transfer1DSetGroups();
    void Transfer1DLoad();
    bool Transfer1DLoad(const std::wstring& strFilename);
    void Transfer1DSave();
    void Transfer1DCopyTo2DTrans();
    void Populate1DTFLibList();
    void Transfer1DAddToLib();
    void Transfer1DSetFromLib();
    void Transfer1DAddFromLib();
    void Transfer1DSubFromLib();
    void Transfer1DConfigureLib();

    void Transfer2DAddGradient();
    void Transfer2DDeleteGradient();
    void Transfer2DChooseGradientColor();
    void Transfer2DChooseGradientOpacity();
    void Transfer2DChooseGradientColorSimpleUI();
    void Transfer2DChooseGradientOpacitySimpleUI();
    void Transfer2DToggleGradientType();
    void Transfer2DLoad();
    bool Transfer2DLoad(const std::wstring& strFilename);
    void Transfer2DSave();
    void Transfer2DToggleTFMode();

    void SetUpdateMode();
    void SetTagVolume();
    void ApplyUpdate();

    void Transfer2DSwatchesChanged();
    void Transfer2DSwatcheTypeChanged(int i);
    void Transfer2DUpdateSwatchButtons();
    void Transfer2DUpdateGradientType();
    void Transfer2DUpdateGradientBox();
    void Transfer2DUpdateGradientButtons();

    void SetHistogramScale1D(int v);
    void SetHistogramScale2D(int v);

    bool LoadWorkspace();
    bool SaveWorkspace();
    bool ApplyWorkspace();
    void ResetToDefaultWorkspace();

    bool LoadGeometry();
    bool SaveGeometry();

    void OpenRecentFile();
    void OpenRecentWSFile();
    void ClearMRUList();
    void ClearWSMRUList();

    void UpdateMenus();
    void ExportDataset();
    void MergeDatasets();
    bool ExportDataset(uint32_t iLODLevel, const std::wstring& targetFileName);
    void ExportIsosurface();
    bool ExportIsosurface(uint32_t iLODLevel, const std::wstring& targetFileName);
    void ExportImageStack();
    bool ExportImageStack(uint32_t iLODLevel, const std::wstring& targetFileName, bool bAllDirs);

    void RenderWindowActive(RenderWindow* sender);
    void RenderWindowClosing(RenderWindow* sender);
    void StereoDisabled();
    void RenderWindowViewChanged(int iMode);
    void EnableStereoWidgets();
    void DisableStereoWidgets();

    void ShowVersions();
    void ShowGPUInfo(bool bWithExtensions);
    void ShowSysInfo();
    void ListSupportedImages();
    void ListSupportedVolumes();
    void ListSupportedGeometry();
    void ClearDebugWin();
    void SetDebugViewMask();

    void CheckForRedraw();
    bool ShowAdvancedSettings(bool bInitializeOnly);
    bool ShowBasicSettings(bool initOnly);
    bool ShowSettings(bool initOnly=false);
    void SetLighting(bool bLighting);
    void ToggleLighting();

    void Collapse2DWidgets();
    void Expand2DWidgets();
    void SetSampleRate(int iValue);
    void SetFoV(int iValue);
    void SetFoVSlider(int iValue);
    void SetIsoValue(float fValue);
    void SetIsoValue(int iValue);
    void SetClearViewIsoValue(float fValue);
    void ToggleGlobalBBox(bool bRenderBBox);
    void ToggleLocalBBox(bool bRenderBBox);
    void ToggleClipPlane(bool);
    void ClipToggleLock(bool);
    void ClipToggleShow(bool);
    void CropData();
    void SetTimestep(int);
    void SetTimestepSlider(int iValue, int iMaxValue);
    void UpdateTimestepLabel(int iValue, int iMaxValue);
    void ResetTimestepUI();

    void SetStayOpen(bool bStayOpenAfterScriptEnd);
    void SetRescaleFactors();
    virtual void closeEvent(QCloseEvent *event);

    // update
    void httpFinished();
    void httpReadyRead();

    void UploadLogToServer();

    void ShowAbout();
    void ChooseIsoColor();

    void ToggleFullscreen();
    void ToggleClearView();
    void SetFocusIsoValue(int iValue);
    void ChooseFocusColor();
    void SetFocusSize(int iValue);
    void SetContextScale(int iValue);
    void SetBorderSize(int iValue);

    void UpdateLockView();
    void LockModalityChange();
    void ChangeLocks();

    void ToggleStereoRendering();
    void ToggleStereoMode();
    void ToggleStereoEyeSwap();
    void SetStereoEyeDistance();
    void SetStereoFocalLength();

    void Show1DTrans();
    void Show2DTrans();
    void ShowIsoEdit();
    void PreserveTransparencyChanged();

    void ShowWelcomeScreen();
    void DisplayMetadata();
    void SaveDefaultWorkspace();
    void SaveDefaultGeometry();

    void SaveAspectRatioToUVF();
    void MinLODLimitChanged();
    void MaxLODLimitChanged();

    void PickLightColor();
    void ChangeLightColors();
    void LightMoved();
    void UpdateExplorerView() {UpdateExplorerView(false);}
    void ToggleMesh();
    void SetMeshDefColor();
    void SetMeshDefOpacity();
    void SetMeshScaleAndBias();
    void SaveMeshTransform(ScaleAndBiasDlg* sender);
    void RestoreMeshTransform(ScaleAndBiasDlg* sender);
    void ApplMeshTransform(ScaleAndBiasDlg* sender);
    void ApplyMatrixMeshTransform(ScaleAndBiasDlg* sender);

  private :
    QTimer*                                   m_pRedrawTimer;
    MasterController&                         m_MasterController;
    QString                                   m_strCurrentWorkspaceFilename;
    Q1DTransferFunction*                      m_1DTransferFunction;
    Q2DTransferFunction*                      m_2DTransferFunction;
    QLightPreview*                            m_pQLightPreview;
    QGLWidget*                                m_glShareWidget;
    QTOut*                                    m_pDebugOut;
    static const unsigned int                 ms_iMaxRecentFiles = 5;
    QAction*                                  m_recentFileActs[ms_iMaxRecentFiles];
    QAction*                                  m_recentWSFileActs[ms_iMaxRecentFiles];
    FLOATVECTOR3                              m_vBackgroundColors[2];
    FLOATVECTOR4                              m_vTextColor;
    std::wstring                              m_strTempDir;
    bool                                      m_bShowVersionInTitle;
    bool                                      m_bQuickopen;
    unsigned int                              m_iMinFramerate;
    unsigned int                              m_iLODDelay;
    bool                                      m_bRenderLowResIntermediateResults;
    unsigned int                              m_iActiveTS;
    unsigned int                              m_iInactiveTS;
    bool                                      m_bWriteLogFile;
    QString                                   m_strLogFileName;
    unsigned int                              m_iLogLevel;

    WelcomeDialog*                            m_pWelcomeDialog;
    MetadataDlg*                              m_pMetadataDialog;
    DebugScriptWindow*                        m_pDebugScriptWindow;

    unsigned int                              m_iBlendPrecisionMode;
    bool                                      m_bPowerOfTwo;
    bool                                      m_bDownSampleTo8Bits;
    bool                                      m_bDisableBorder;
    bool                                      m_bI3MFeatures;
    bool                                      m_bAdvancedSettings;
    bool                                      m_bAutoSaveGEO;
    bool                                      m_bAutoSaveWSP;
    MasterController::EVolumeRendererType     m_eVolumeRendererType;
    bool                                      m_bUpdatingLockView;
    QString                                   m_strLogoFilename;
    int                                       m_iLogoPos;
    bool                                      m_bAutoLockClonedWindow;
    bool                                      m_bAbsoluteViewLocks;
    bool                                      m_bCheckForUpdatesOnStartUp;
    bool                                      m_bCheckForDevBuilds;
    bool                                      m_bShowWelcomeScreen;
    bool                                      m_bInvWheel;

    bool                                      m_bStayOpenAfterScriptEnd;

    void AddGeometry(const std::wstring& filename);

    RenderWindow* WidgetToRenderWin(QWidget* w);
    RenderWindow* CreateNewRenderWindow(QString dataset);
    bool CheckRenderwindowFitness(RenderWindow *renderWin,
                                  bool bIfNotOkShowMessageAndCloseWindow=true);

    void SetupWorkspaceMenu();
    bool LoadWorkspace(QString strFilename,
           bool bSilentFail = false,
           bool bRetryResource = true);

    bool SaveWorkspace(QString strFilename);

    bool LoadGeometry(QString strFilename,
          bool bSilentFail = false,
          bool bRetryResource = true);

    bool SaveGeometry(QString strFilename);

    bool LoadDefaultWorkspace();
    bool LoadDefaultGeometry();

    void SetTitle();
    void setupUi(QMainWindow *MainWindow);


    void UpdateWSMRUActions();
    void AddFileToWSMRUList(const QString &fileName);

    void UpdateMRUActions();
    void AddFileToMRUList(const QString &fileName);

    void GetDebugViewMask();

    bool LoadDataset(const std::vector< std::wstring >& strParams);
    bool LoadDataset(QStringList fileName, QString targetFileName="",
                     bool bNoUserInteraction=false);
    bool LoadDatasetInternal(QStringList files, QString targetFilename,
                             bool bNoUserInteraction);
    RenderWindow* LuaLoadDatasetInternal(const std::vector<std::wstring>& filename,
                                         const std::wstring& targetFileName,
                                         bool bNoUserInteraction);
    bool RebrickDataset(QString filename, QString targetFilename,
                        bool bNoUserInteraction);

    QString GetConvFilename(const QString& sourceName = "");

    void InitDockWidget(QDockWidget * v) const;
    void InitAllWorkspaces();

    void CheckSettings();
    void ApplySettings();
    void ApplySettings(RenderWindow* renderWin);
    void SaveSettings(
      uint64_t CPUMem, uint64_t maxCPU, uint64_t GPUMem,
      uint64_t maxGPU, bool ignoreMax,
      const std::wstring& tempDir,
      bool checksum, unsigned framerate, bool lowResSubframes, unsigned LODDelay,
      unsigned activeTS, unsigned inactiveTS, bool writeLog, bool showCrashDialog,
      const std::wstring& logFile, uint32_t logLevel, bool showVersion,
      bool autoSaveGeo, bool autoSaveWSP, bool autoLockCloned, bool absoluteLocks,
      bool checkForUpdates,
      bool checkForDevBuilds, bool showWelcome, bool invertWheel, bool i3mFeatures,
      unsigned volRenType, unsigned blendPrecision, bool powerTwo,
      bool downsampleTo8, bool disableBorder, const FLOATVECTOR3& backColor1,
      const FLOATVECTOR3& backColor2, const FLOATVECTOR4& textColor,
      const QString& logo, int logoPosition, unsigned maxBrickSize,
      unsigned builderBrickSize, bool medianFilter, bool clampToEdge,
      uint32_t compression, uint32_t compressionLevel,
      bool experimentalFeatures, bool advancedSettings, uint32_t layout
    );
    void SetSampleRateSlider(int iValue);
    void UpdateSampleRateLabel(int iValue);
    void UpdateFoVLabel(int iValue);
    void SetIsoValueSlider(int iValue, int iMaxValue);
    void UpdateIsoValLabel(int iValue, int iMaxValue);
    void SetToggleGlobalBBoxLabel(bool bRenderBBox);
    void SetToggleLocalBBoxLabel(bool bRenderBBox);
    void SetToggleClipEnabledLabel(bool);
    void SetToggleClipShownLabel(bool);
    void SetToggleClipLockedLabel(bool);
    void ClearProgressViewAndInfo();

    void ToggleClearViewControls();
    void ToggleClearViewControls(int iRange);
    void SetFocusIsoValueSlider(int iValue, int iMaxValue);
    void UpdateFocusIsoValLabel(int iValue, int iMaxValue);
    void SetFocusSizeValueSlider(int iValue);
    void SetContextScaleValueSlider(int iValue);
    void SetBorderSizeValueSlider(int iValue);

    void CompareFiles(const std::wstring& strFile1, const std::wstring& strFile2) const;

    void RemoveAllLocks(RenderWindow* sender);
    void RemoveAllLocks(RenderWindow* sender, size_t iLockType);
    bool SetLock(size_t iLockType, RenderWindow* winA, RenderWindow* winB);
    bool IsLockedWith(size_t iLockType, RenderWindow* winA, RenderWindow* winB);

    bool CaptureFrame(const std::wstring& strTargetName);
    bool CaptureSequence(const std::wstring& strTargetName, std::wstring* strRealFilename=NULL);

    void RotateCurrentViewX(double angle);
    void RotateCurrentViewY(double angle);
    void RotateCurrentViewZ(double angle);
    void TranslateCurrentView(double x, double y, double z);
    void ResetRenderingParameters();

    void ShowInformationDialog(QString strTitle, QString strMessage);
    void ShowWarningDialog(QString strTitle, QString strMessage);
    void ShowCriticalDialog(QString strTitle, QString strMessage);

    void SetAndCheckRunningFlag();
    void RemoveRunningFlag();
    void ReportABug(const std::wstring& strFile);
    void ToggleLogFile();

    void UpdatePolyTypeLabel(int iCurrent);

    void UpdateMinMaxLODLimitLabel();
    void UpdateExplorerView(bool bRepopulateListBox);
    void UpdateTFScaleSliders();
    void UpdateInterpolant();
    void UpdateColorWidget();

    void LuaSetIsoValueFloat(float fValue);
    void LuaSetIsoValueInteger(int iValue);

    void LuaMoveProgramWindow(const INTVECTOR2& pos);
    void LuaResizeProgramWindow(const UINTVECTOR2& size);

    void RegisterLuaClasses();

    std::wstring ConvertTF(const std::wstring& strSource1DTFilename,
                          const std::wstring& strTargetDir,
                          const std::wstring& strTargetFullFilename,
                          PleaseWaitDialog& pleaseWait);

    std::wstring ConvertDataToI3M(LuaClassInstance currentDataset,
                                 const std::wstring& strTargetDir,
                                 PleaseWaitDialog& pleaseWait,
                                 bool bOverrideExisting);
    bool ExportGeometry(size_t i, const std::wstring& strFilename);

    TextfileOut* m_pTextout;

    RenderWindow* ActiveRenderWin();
    QMdiSubWindow* ActiveSubWindow();
    RenderWindow* m_pActiveRenderWin;
    RenderWindow* m_pLastLoadedRenderWin; ///< Use with caution -- could have
                                          ///< been destroyed. Used to return
                                          ///< render window.

    // update
    struct VersionNumber {
      size_t major;
      size_t minor;
      size_t patch;
      size_t svn; ///< revision
      /// Compares only version #'s, ignores svn revision.
      ///@{
      bool operator ==(const struct VersionNumber &) const;
      bool operator > (const struct VersionNumber &) const;
      ///@}
      /// Converts major/minor/patch into a string.
      operator std::string() const;
    };
    QFile* m_pUpdateFile;
    QNetworkAccessManager m_Http;
    QNetworkReply *m_pHttpReply;


    bool   m_bStartupCheck;
    bool   m_bScriptMode;
    void CheckForUpdatesInternal();
    void QuietCheckForUpdates();
    bool GetVersionsFromUpdateFile(const std::wstring& strFilename,
                                   struct VersionNumber& iv3d,
                                   struct VersionNumber& tuvok);
    void DeleteUpdateFile();
    bool CheckForMeshCapabilities(bool bNoUserInteraction, 
                                  QStringList files=QStringList(""));

    // ftp
    FTPDialog*   m_pFTPDialog;
    std::wstring m_strFTPTempFile;
    bool         m_bFTPDeleteSource;
    bool         m_bFTPFinished;
    bool         FtpTransfer(const std::wstring& strSource, const std::wstring& strDest, bool bDeleteSource = true);
    QString      GenUniqueName(const std::wstring& strPrefix, const std::wstring& strExt=L"txt");
    bool         Pack(const std::vector< std::wstring >& strParams);
    void SetStereoMode(unsigned int iMode);
    void SetStereoFocalLength(float fLength);
    void SetStereoEyeDistance(float fEyeDist);

    bool m_bClipDisplay;
    bool m_bClipLocked;

    bool m_bIgnoreLoadDatasetFailure;

    tuvok::LuaMemberReg       m_MemReg;

    friend class MergeDlg;
};

#endif // IMAGEVIS3D_H
