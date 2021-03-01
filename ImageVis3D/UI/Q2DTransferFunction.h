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


//!    File   : Q2DTransferFunction.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#pragma once

#ifndef Q2DTRANSFERFUNCTION
#define Q2DTRANSFERFUNCTION

#include "QTransferFunction.h"
#include "../Tuvok/IO/TransferFunction2D.h"
#include <QtGui/QMouseEvent>

#define Q2DT_PAINT_NONE  0
#define Q2DT_PAINT_RED   1
#define Q2DT_PAINT_GREEN 2
#define Q2DT_PAINT_BLUE  4
#define Q2DT_PAINT_ALPHA 8
#define Q2DT_PAINT_UNDEF 16

enum EDragMode {
  DRM_MOVE,
  DRM_ROTATE,
  DRM_SCALE,
  DRM_MOVE_ZOOM,
  DRM_NONE
};

enum E2DTransferFunctionMode {
  TFM_EXPERT = 0,
  TFM_BASIC,
  TFM_INVALID
};

enum E2DSimpleModePolyType {
  PT_PSEUDOTRIS = 0,
  PT_RECTANGLE,
  PT_OTHER
};

enum EQ2DSimpleEDragMode {
  SDM_POLY,
  SDM_EDGE,
  SDM_VERTEX,
  SDM_GRAD_CENTER,
  SDM_NONE
};

class SimpleSwatchInfo {
public:
  SimpleSwatchInfo(const E2DSimpleModePolyType& eType=PT_OTHER,
                   const FLOATVECTOR2& vHandlePos=FLOATVECTOR2(0,0),
                   const std::string& desc="" ) :
    m_eType(eType),
    m_vHandlePos(vHandlePos),
    m_strDesc(desc)
  {}

  E2DSimpleModePolyType m_eType;
  FLOATVECTOR2          m_vHandlePos;
  std::string           m_strDesc;
};

class Q2DTransferFunction : public QTransferFunction
{
  Q_OBJECT

public:
  Q2DTransferFunction(MasterController& masterController, QWidget *parent=0);
  virtual ~Q2DTransferFunction(void);
  void SetData(std::shared_ptr<const Histogram2D> vHistogram, 
               LuaClassInstance trans);
  void SetPaintmode(unsigned int iPaintmode) {
    if (iPaintmode < Q2DT_PAINT_UNDEF) m_iPaintmode = iPaintmode;};
  void Set1DTrans(LuaClassInstance p1DTrans);

  QSize minimumSizeHint() const;
  QSize sizeHint() const;

  int GetActiveSwatchIndex() {
    return m_iActiveSwatchIndex;
  }
  size_t GetSwatchCount();
  size_t GetSwatchSize(unsigned int i);

  bool GetActiveGradientType();
  void SetActiveGradientType(bool bRadial);

  size_t GetGradientCount();
  GradientStop GetGradient(unsigned int i);

  void AddGradient(GradientStop stop);
  void DeleteGradient(unsigned int i);
  void SetGradient(unsigned int i, GradientStop stop);

  virtual void ApplyFunction();

  void Toggle2DTFMode();
  void Set2DTFMode(E2DTransferFunctionMode TransferFunctionMode);
  E2DTransferFunctionMode Get2DTFMode() const { return m_eTransferFunctionMode;}
  std::string GetSwatchDesciption() const;

public slots:
  void Transfer2DSetActiveSwatch(const int iActiveSwatch);
  void Transfer2DAddSwatch();
  void Transfer2DAddCircleSwatch();

  void Transfer2DAddRectangleSwatch();
  void Transfer2DAddPseudoTrisSwatch();
  void Transfer2DDeleteSwatch();
  void Transfer2DUpSwatch();
  void Transfer2DDownSwatch();

  bool LoadFromFile(const QString& strFilename);
  bool SaveToFile(const QString& strFilename);

signals:
  void SwatchChange();
  void SwatchTypeChange(int i);

protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void wheelEvent(QWheelEvent *event);
  virtual void changeEvent(QEvent * event);
  virtual void resizeEvent( QResizeEvent * event );
  virtual void keyPressEvent( QKeyEvent * event );
  virtual void keyReleaseEvent( QKeyEvent * event );

private:
  // states
  NormalizedHistogram2D    m_vHistogram;
  LuaClassInstance         m_trans;
  unsigned int             m_iPaintmode;
  int                      m_iActiveSwatchIndex;
  E2DTransferFunctionMode m_eTransferFunctionMode;

  // cached image of the backdrop
  unsigned int m_iCachedHeight;
  unsigned int m_iCachedWidth;
  QPixmap*   m_pBackdropCache;

  // cached image of the histogram
  QImage* m_pHistImage;

  // border size, may be changed in the constructor
  unsigned int m_iSwatchBorderSize;

  // colors, may be changed in the setcolor routine
  QColor m_colorHistogram;
  QColor m_colorBack;
  QColor m_colorSwatchBorder;
  QColor m_colorSwatchBorderHighlight;
  QColor m_colorSwatchBorderHighlightCenter;
  QColor m_colorSwatchBorderInactive;
  QColor m_colorSwatchBorderInactiveHighlight;
  QColor m_colorSwatchBorderCircle;
  QColor m_colorSwatchBorderCircleHighlight;
  QColor m_colorSwatchGradCircle;
  QColor m_colorSwatchBorderCircleSel;
  QColor m_colorSwatchGradCircleSel;

  void SetColor(bool bIsEnabled);

  // mouse motion handling
  int      m_iPointSelIndex;
  int      m_iGradSelIndex;
  INTVECTOR2  m_vMousePressPos;
  bool    m_bDragging;
  bool    m_bDraggingAll;
  EDragMode  m_eDragMode;
  FLOATVECTOR4  m_vZoomWindow;
  Qt::MouseButton m_mouseButton;

  // drawing routines
  void ClearToBlack(QPainter& painter);
  void DrawHistogram(QPainter& painter);
  void DrawSwatches(QPainter& painter);
  void DrawSwatcheDecoration(QPainter& painter);
  void Draw1DTrans(QPainter& painter);

  void GenerateHistogramImage();
  void ComputeCachedImageSize(uint32_t &w , uint32_t &h) const;

  // helper
  INTVECTOR2   Normalized2Offscreen(FLOATVECTOR2 vfCoord) const;
  INTVECTOR2   Normalized2Screen(FLOATVECTOR2 vfCoord) const;
  FLOATVECTOR2 Screen2Normalized(INTVECTOR2   vCoord) const;
  FLOATVECTOR2 Rotate(FLOATVECTOR2 point,
          float angle,
          FLOATVECTOR2 center,
          FLOATVECTOR2 rescale);

  void SetDragMode(bool bShiftPressed, bool bCtrlPressed);
  void DragInit(INTVECTOR2 vMousePressPos, Qt::MouseButton mouseButton);

  void DrawPolygonWithCool3DishBorder(QPainter& painter, std::vector<QPoint>& pointList, QPen& borderPen, QPen& borderPenHighlight);

  // Lua helper functions
  std::shared_ptr<const std::vector<TFPolygon> > GetSwatches() const;

  // For simple mode
  SimpleSwatchInfo ClassifySwatch(TFPolygon& polygon) const;
  void ComputeGradientForPseudoTris(TFPolygon& swatch, const FLOATVECTOR4& color);
  void RecomputeLowerPseudoTrisPoints(TFPolygon& currentSwatch, const FLOATVECTOR2& vHandePos);
  bool PointInPolygon(const FLOATVECTOR2& point, const TFPolygon& poly) const;
  std::vector<SimpleSwatchInfo> m_vSimpleSwatchInfo;
  EQ2DSimpleEDragMode m_eSimpleDragMode;
  int m_iSimpleDragModeSubindex;
  void UpdateSwatchType(size_t i);
  void UpdateSwatchTypes();
  int PickSwatch(const FLOATVECTOR2& pickPos) const;
  int PickEdge(const FLOATVECTOR2& pickPos, int& iEdgeIndex) const;
  int PickVertex(const FLOATVECTOR2& pickPos, int& iVertexIndex) const;
  int PickGradient(const FLOATVECTOR2& pickPos) const;
  void DrawPolyVertex(QPainter& painter, QPoint& p, bool bBorderVertex=true);
  void DrawPolyVertex(QPainter& painter, const INTVECTOR2& p, bool bBorderVertex=true);
};


#endif // Q2DTRANSFERFUNCTION
