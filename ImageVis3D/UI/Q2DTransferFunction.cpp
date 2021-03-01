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


//!    File   : Q2DTransferFunction.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include <exception>
#include <limits>
#include "Q2DTransferFunction.h"
#include <QtGui/QPainter>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTuvokTypes.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTransferFun1DProxy.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTransferFun2DProxy.h"

#ifdef max
  #undef max
#endif

#ifdef min
  #undef min
#endif

using namespace std;

Q2DTransferFunction::Q2DTransferFunction(MasterController& masterController, QWidget *parent) :
  QTransferFunction(masterController, parent),
  m_iPaintmode(Q2DT_PAINT_NONE),
  m_iActiveSwatchIndex(-1),
  m_eTransferFunctionMode(TFM_EXPERT),
  m_iCachedHeight(0),
  m_iCachedWidth(0),
  m_pBackdropCache(NULL),
  m_pHistImage(NULL),

  // border size, may be changed arbitrarily
  m_iSwatchBorderSize(3),

  // mouse motion
  m_iPointSelIndex(-1),
  m_iGradSelIndex(-1),
  m_vMousePressPos(0,0),
  m_bDragging(false),
  m_bDraggingAll(false),
  m_eDragMode(DRM_NONE),
  m_vZoomWindow(0.0f,0.0f,1.0f,1.0f),
  m_eSimpleDragMode(SDM_NONE),
  m_iSimpleDragModeSubindex(0)
{
  SetColor(isEnabled());

  setFocusPolicy(Qt::StrongFocus); 
}

Q2DTransferFunction::~Q2DTransferFunction(void)
{
  // delete the cache pixmap and image
  delete m_pBackdropCache;
  delete m_pHistImage;
}

QSize Q2DTransferFunction::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize Q2DTransferFunction::sizeHint() const
{
  return QSize(400, 400);
}

void Q2DTransferFunction::SetData(shared_ptr<const Histogram2D> vHistogram,
                                  LuaClassInstance tf2d) {
  m_trans = tf2d;
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (m_trans.isValid(ss) == false) {
    return;
  }

  // resize the histogram vector
  m_vHistogram.Resize(vHistogram->GetSize());

  // if the histogram is empty we are done
  if (m_vHistogram.GetSize().area() == 0)  return;

  // rescale the histogram to the [0..1] range
  // first find min and max ...
  unsigned int iMax = vHistogram->GetLinear(0);
  unsigned int iMin = iMax;
  for (size_t i = 0;i<m_vHistogram.GetSize().area();i++) {
    unsigned int iVal = vHistogram->GetLinear(i);
    if (iVal > iMax) iMax = iVal;
    if (iVal < iMin) iMin = iVal;
  }

  // ... than rescale
  float fDiff = float(iMax)-float(iMin);
  for (size_t i = 0;i<m_vHistogram.GetSize().area();i++)
    m_vHistogram.SetLinear(i, (float(vHistogram->GetLinear(i)) - float(iMin)) / fDiff);

  // Upload the new TF to the GPU.
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);

  // force the draw routine to recompute the backdrop cache
  m_bHistogramChanged = true;

  emit SwatchChange();
}


void Q2DTransferFunction::GenerateHistogramImage() {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // convert the histogram into an image
  // define the bitmap ...
  if (!m_pHistImage ||
      static_cast<size_t>(m_pHistImage->height()) != m_vHistogram.GetSize().x ||
      static_cast<size_t>(m_pHistImage->width()) != m_vHistogram.GetSize().y) {
    delete m_pHistImage;
    m_pHistImage = new QImage(QSize(int(m_vHistogram.GetSize().x), int(m_vHistogram.GetSize().y)), QImage::Format_RGB32);
  }

  for (size_t y = 0;y<m_vHistogram.GetSize().y;y++)
    for (size_t x = 0;x<m_vHistogram.GetSize().x;x++) {
      float value = min<float>(1.0f, pow(m_vHistogram.Get(x,y),1.0f/(1+(m_fHistfScale-1)/100.0f)));
      m_pHistImage->setPixel(int(x),
         int(m_vHistogram.GetSize().y-(y+1)),
         qRgb(int(m_colorBack.red()  * (1.0f-value) +
                  m_colorHistogram.red()  * value),
              int(m_colorBack.green()* (1.0f-value) +
                  m_colorHistogram.green()* value),
              int(m_colorBack.blue() * (1.0f-value) +
                  m_colorHistogram.blue() * value)));
    }

  m_bBackdropCacheUptodate = false;
  m_bHistogramChanged = false;
}

void Q2DTransferFunction::DrawHistogram(QPainter& painter) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  if (m_bHistogramChanged) GenerateHistogramImage();

  // ... draw it
  QRectF target(0, 0,
    painter.viewport().width(), painter.viewport().height());
  QRectF source(m_vZoomWindow.x * m_vHistogram.GetSize().x,
                m_vZoomWindow.y * m_vHistogram.GetSize().y,
                m_vZoomWindow.z * m_vHistogram.GetSize().x,
                m_vZoomWindow.w * m_vHistogram.GetSize().y);
  painter.drawImage( target, *m_pHistImage, source );
}

INTVECTOR2 Q2DTransferFunction::Normalized2Offscreen(FLOATVECTOR2 vfCoord) const {
  return INTVECTOR2(int(-m_vZoomWindow.x/m_vZoomWindow.z * m_iCachedWidth +vfCoord.x/m_vZoomWindow.z*m_iCachedWidth),
                    int(-m_vZoomWindow.y/m_vZoomWindow.w * m_iCachedHeight+vfCoord.y/m_vZoomWindow.w*m_iCachedHeight));
}

INTVECTOR2 Q2DTransferFunction::Normalized2Screen(FLOATVECTOR2 vfCoord) const {
  return INTVECTOR2(int(-m_vZoomWindow.x/m_vZoomWindow.z * width() +vfCoord.x/m_vZoomWindow.z*width()),
                    int(-m_vZoomWindow.y/m_vZoomWindow.w * height()+vfCoord.y/m_vZoomWindow.w*height()));
}

FLOATVECTOR2 Q2DTransferFunction::Screen2Normalized(INTVECTOR2 vCoord) const {
  return FLOATVECTOR2(float(vCoord.x)*m_vZoomWindow.z/width()+m_vZoomWindow.x,
                      float(vCoord.y)*m_vZoomWindow.w/height()+m_vZoomWindow.y);
}

SimpleSwatchInfo Q2DTransferFunction::ClassifySwatch(TFPolygon& polygon) const {
  // check the most basic properties first (four vertices, linear gradient, and exactly 3 gradient stops)
  if (polygon.pPoints.size() == 4 && !polygon.bRadial && polygon.pGradientStops.size() == 3) {

    // check if the top and bottom edge are parallel to the x-axis
    if (fabs(polygon.pPoints[1].y - polygon.pPoints[2].y) < 0.01 &&
        fabs(polygon.pPoints[0].y - polygon.pPoints[3].y) < 0.01) {

      // snap points if they are close but really the same
      polygon.pPoints[1].y = polygon.pPoints[2].y;
      polygon.pPoints[0].y = polygon.pPoints[3].y;

      // check if the left and right edge are parallel to the y-axis -> rectangle
      if (fabs(polygon.pPoints[0].x - polygon.pPoints[1].x) < 0.01 &&
          fabs(polygon.pPoints[2].x - polygon.pPoints[3].x) < 0.01 ) {

        // snap points if they are close but really the same
        polygon.pPoints[0].x = polygon.pPoints[1].x;
        polygon.pPoints[2].x = polygon.pPoints[3].x;

        return SimpleSwatchInfo(PT_RECTANGLE,FLOATVECTOR2(0,0),"Rectangle");;
      } else {
        // check if the left and right edge are intersecting "near" the x-axis

        double x1 = polygon.pPoints[0].x, y1 = polygon.pPoints[0].y;
        double x2 = polygon.pPoints[1].x, y2 = polygon.pPoints[1].y;
        double x3 = polygon.pPoints[2].x, y3 = polygon.pPoints[2].y;
        double x4 = polygon.pPoints[3].x, y4 = polygon.pPoints[3].y;

        double u = ((x4-x3)*(y1-y3)-(y4-y3)*(x1-x3))/((y4-y3)*(x2-x1)-(x4-x3)*(y2-y1));

        double h = y1 + u *(y2-y1);

        if (fabs(h-1.0f) < 0.01) {

          // when an "other primitive" snaps to a triangle, the edges may be in the wrong order
          if (polygon.pPoints[1].y > polygon.pPoints[0].y) {
            std::swap(polygon.pPoints[0],polygon.pPoints[1]);
            std::swap(polygon.pPoints[2],polygon.pPoints[3]);
          }

          return SimpleSwatchInfo(PT_PSEUDOTRIS,FLOATVECTOR2(x1 + u *(x2-x1),1.0),"Triangle");
        } else {
          return SimpleSwatchInfo(PT_OTHER,FLOATVECTOR2(0,0),"Polygon (wrong distance to lower bound)");
        }
      }
    }
  }

  string otherDesc = "Polygon (";
  if (polygon.pPoints.size() != 4) otherDesc += "not a quadrilateral, ";
    else {
      if (fabs(polygon.pPoints[1].y - polygon.pPoints[2].y) > 0.01 ||
          fabs(polygon.pPoints[0].y - polygon.pPoints[3].y) > 0.01) otherDesc += "not x-axis aligned, ";
      if (fabs(polygon.pPoints[0].x - polygon.pPoints[1].x) > 0.01 ||
          fabs(polygon.pPoints[2].x - polygon.pPoints[3].x) > 0.01) otherDesc += "not y-axis aligned, ";
    }
  if (polygon.bRadial) otherDesc += "radial gradient, ";
  if (polygon.pGradientStops.size() != 3) otherDesc += "custom gradient, ";
  otherDesc = otherDesc.substr(0,otherDesc.size()-2) + ")";

  return SimpleSwatchInfo(PT_OTHER,FLOATVECTOR2(0,0),otherDesc);
}

void Q2DTransferFunction::DrawPolygonWithCool3DishBorder(QPainter& painter, std::vector<QPoint>& pointList, QPen& borderPen, QPen& borderPenHighlight) {
  painter.setPen(borderPen);
  painter.setBrush(Qt::NoBrush);
  painter.drawPolygon(&pointList[0], int(pointList.size()));
  painter.setPen(borderPenHighlight);
  painter.drawPolygon(&pointList[0], int(pointList.size()));
}

void Q2DTransferFunction::DrawPolyVertex(QPainter& painter, QPoint& p, bool bBorderVertex) {
  DrawPolyVertex(painter, INTVECTOR2(p.x(), p.y()),bBorderVertex);
}

void Q2DTransferFunction::DrawPolyVertex(QPainter& painter, const INTVECTOR2& p, bool bBorderVertex) {
  if (bBorderVertex)
    painter.drawEllipse(p.x-m_iSwatchBorderSize, p.y-m_iSwatchBorderSize, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
  else
    painter.drawEllipse(p.x, p.y, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
}

void Q2DTransferFunction::DrawSwatcheDecoration(QPainter& painter) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // if we start in simple mode we need to build the swatch type list in the first draw call
  if (m_vSimpleSwatchInfo.size() != GetSwatchCount()) UpdateSwatchTypes();

  painter.setRenderHint(painter.Antialiasing, true);
//  painter.translate(+0.5, +0.5);  /// \todo check if we need this

  QPen borderPen(m_colorSwatchBorder,         m_iSwatchBorderSize, Qt::SolidLine);
  QPen borderPenHighlight(m_colorSwatchBorderHighlight, m_iSwatchBorderSize/2, Qt::SolidLine);
  QPen borderPenHighlightCenter(m_colorSwatchBorderHighlightCenter, 1, Qt::SolidLine);
  QPen inactiveBorderPen(m_colorSwatchBorderInactive, m_iSwatchBorderSize, Qt::SolidLine);
  QPen inactiveBorderHighlight(m_colorSwatchBorderInactiveHighlight, m_iSwatchBorderSize/2, Qt::SolidLine);
  QPen noBorderPen(Qt::NoPen);
  QPen circlePen(m_colorSwatchBorderCircle, m_iSwatchBorderSize, Qt::SolidLine);
  QPen circlePenHighlight(m_colorSwatchBorderCircleHighlight, m_iSwatchBorderSize/2, Qt::SolidLine);
  QPen gradCircePen(m_colorSwatchGradCircle, m_iSwatchBorderSize/2, Qt::SolidLine);
  QPen circlePenSel(m_colorSwatchBorderCircleSel, m_iSwatchBorderSize, Qt::SolidLine);
  QPen gradCircePenSel(m_colorSwatchGradCircleSel, m_iSwatchBorderSize/2, Qt::SolidLine);

  QBrush solidBrush = QBrush(m_colorSwatchBorderCircle, Qt::SolidPattern);

  // Obtain swatch vector from Lua.
  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();

  // render swatches
  for (size_t i = 0;i<GetSwatchCount();i++) {
    const TFPolygon& currentSwatch = (*swatches)[i];

    std::vector<QPoint> pointList(currentSwatch.pPoints.size());
    for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
      INTVECTOR2 vPixelPos = Normalized2Screen(currentSwatch.pPoints[j]);
      pointList[j] = QPoint(vPixelPos.x, vPixelPos.y);
    }


    if (m_eTransferFunctionMode == TFM_BASIC) {
      // for the simple Seg3D like interface to work we classify polygons
      // into three categories:
      //  1. rectangles
      //  2. the "seg3d triangle" which really is a trapezoid
      //  3. anything else (in particular any polygon that is not a quad)

      FLOATVECTOR2 vHandle           = m_vSimpleSwatchInfo[i].m_vHandlePos;
      E2DSimpleModePolyType polyType = m_vSimpleSwatchInfo[i].m_eType;

      if (m_iActiveSwatchIndex == int(i)) {
        DrawPolygonWithCool3DishBorder(painter, pointList, borderPen,
                                       borderPenHighlight);
      } else {
        DrawPolygonWithCool3DishBorder(painter,  pointList, inactiveBorderPen,
                                       inactiveBorderHighlight);
      }

      switch (polyType) {
        case PT_PSEUDOTRIS:
          {
            INTVECTOR2 vPixelPos = Normalized2Screen(vHandle);

            painter.setPen(circlePen);
            DrawPolyVertex(painter, pointList[1]);
            DrawPolyVertex(painter, pointList[2]);

            pointList[1] = pointList[3];
            pointList[2] = QPoint(vPixelPos.x, vPixelPos.y);
            pointList.pop_back();
            if (m_iActiveSwatchIndex == int(i)) {
              DrawPolygonWithCool3DishBorder(painter, pointList, borderPen,
                                             borderPenHighlight);
            } else {
              DrawPolygonWithCool3DishBorder(painter,  pointList,
                                             inactiveBorderPen,
                                             inactiveBorderHighlight);
            }
          }
          break;
        case PT_RECTANGLE:
          painter.setPen(gradCircePen);
          DrawPolyVertex(
            painter, Normalized2Screen(
              currentSwatch.pGradientCoords[0] *
              (1-currentSwatch.pGradientStops[1].first) +
              currentSwatch.pGradientCoords[1] *
              currentSwatch.pGradientStops[1].first
            )
          );
          break;
        case PT_OTHER:
          painter.setPen(circlePen);
          for(size_t j=0; j < pointList.size(); ++j) {
            DrawPolyVertex(painter, pointList[j]);
          }
          break;
        default : break;
      }
    } else {
      // shape
      if (m_iActiveSwatchIndex == int(i)) {
        DrawPolygonWithCool3DishBorder(painter, pointList, borderPen,
                                       borderPenHighlight);
      } else {
        DrawPolygonWithCool3DishBorder(painter,  pointList, inactiveBorderPen,
                                       inactiveBorderHighlight);
      }

      // vertices
      painter.setBrush(solidBrush);
      for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
        if (m_iActiveSwatchIndex == int(i) && m_iPointSelIndex == int(j))
          painter.setPen(circlePenSel);
        else
          painter.setPen(circlePen);
        DrawPolyVertex(painter,pointList[j]);

        if (m_iActiveSwatchIndex != int(i) || m_iPointSelIndex != int(j)) {
          painter.setPen(circlePenHighlight);
          DrawPolyVertex(painter,pointList[j]);
        }
      }

      // gradient coords
      if (m_iActiveSwatchIndex == int(i)) {
        painter.setBrush(Qt::NoBrush);
        for (int j = 0;j<2;j++) {
          if (m_iGradSelIndex==j)
            painter.setPen(gradCircePenSel);
          else
            painter.setPen(gradCircePen);
          INTVECTOR2 vPixelPos = Normalized2Screen(currentSwatch.pGradientCoords[j])-INTVECTOR2(m_iSwatchBorderSize,m_iSwatchBorderSize);
          DrawPolyVertex(painter, vPixelPos, false);
        }
      }

    }
  }

  painter.setRenderHint(painter.Antialiasing, false);
}

void Q2DTransferFunction::DrawSwatches(QPainter& painter) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  QPen noBorderPen(Qt::NoPen);
  QPen circlePen(m_colorSwatchBorderCircle, m_iSwatchBorderSize, Qt::SolidLine);
  QPen gradCircePen(m_colorSwatchGradCircle, m_iSwatchBorderSize/2, Qt::SolidLine);
  QPen circlePenSel(m_colorSwatchBorderCircleSel, m_iSwatchBorderSize, Qt::SolidLine);
  QPen gradCircePenSel(m_colorSwatchGradCircleSel, m_iSwatchBorderSize/2, Qt::SolidLine);

  painter.setPen(noBorderPen);

  QBrush solidBrush = QBrush(m_colorSwatchBorderCircle, Qt::SolidPattern);

  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();

  // render swatches
  for (size_t i = 0;i<GetSwatchCount();i++) {
    const TFPolygon& currentSwatch = (*swatches)[i];

    std::vector<QPoint> pointList(currentSwatch.pPoints.size());
    for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
      INTVECTOR2 vPixelPos = Normalized2Offscreen(currentSwatch.pPoints[j]);
      pointList[j] = QPoint(vPixelPos.x, vPixelPos.y);
    }

    INTVECTOR2 vPixelPos0 = Normalized2Offscreen(currentSwatch.pGradientCoords[0]),
                   vPixelPos1 = Normalized2Offscreen(currentSwatch.pGradientCoords[1]);

    QGradient* pGradientBrush;
    if (currentSwatch.bRadial) {
      double r = sqrt( pow(double(vPixelPos0.x-vPixelPos1.x),2.0) + pow(double(vPixelPos0.y-vPixelPos1.y),2.0));
      pGradientBrush = new QRadialGradient(vPixelPos0.x, vPixelPos0.y, r);
    } else {
      pGradientBrush = new QLinearGradient(vPixelPos0.x, vPixelPos0.y, vPixelPos1.x, vPixelPos1.y);
    }

    for (size_t j = 0;j<currentSwatch.pGradientStops.size();j++) {
      pGradientBrush->setColorAt(currentSwatch.pGradientStops[j].first,
                   QColor(int(currentSwatch.pGradientStops[j].second[0]*255),
                          int(currentSwatch.pGradientStops[j].second[1]*255),
                          int(currentSwatch.pGradientStops[j].second[2]*255),
                          int(currentSwatch.pGradientStops[j].second[3]*255)));
    }

    painter.setBrush(*pGradientBrush);
    painter.drawPolygon(&pointList[0], int(currentSwatch.pPoints.size()));
    delete pGradientBrush;
    painter.setBrush(Qt::NoBrush);
  }
}

void Q2DTransferFunction::SetDragMode(bool bShiftPressed, bool bCtrlPressed) {
  if (bShiftPressed)
    if(bCtrlPressed)
      m_eDragMode = DRM_ROTATE;
    else
      m_eDragMode = DRM_MOVE;
  else
    if(bCtrlPressed)
      m_eDragMode = DRM_SCALE;
    else
      m_eDragMode = DRM_NONE;

}

void Q2DTransferFunction::keyReleaseEvent(QKeyEvent *event) {
  SetDragMode( event->modifiers() & Qt::ShiftModifier,
               event->modifiers() & Qt::ControlModifier);
  DragInit(m_vMousePressPos, m_mouseButton);
}


void Q2DTransferFunction::keyPressEvent(QKeyEvent *event) {
  SetDragMode( event->modifiers() & Qt::ShiftModifier,
               event->modifiers() & Qt::ControlModifier);
  DragInit(m_vMousePressPos, m_mouseButton);
}


void Q2DTransferFunction::DragInit(INTVECTOR2 vMousePressPos, Qt::MouseButton mouseButton) {
  m_vMousePressPos = vMousePressPos;
  m_mouseButton = mouseButton;

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();

  if (m_eTransferFunctionMode == TFM_EXPERT) {

    shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
    if (m_iActiveSwatchIndex >= 0 && m_iActiveSwatchIndex<int(GetSwatchCount())) {
      const TFPolygon& currentSwatch = (*swatches)[m_iActiveSwatchIndex];

      // left mouse drags points around
      if (mouseButton == Qt::LeftButton) {

        m_bDragging = true;
        m_bDraggingAll = m_eDragMode != DRM_NONE;

        m_iPointSelIndex = -1;
        m_iGradSelIndex = -1;

        FLOATVECTOR2 vfP = Screen2Normalized(m_vMousePressPos);
        // find closest corner point
        float fMinDist = std::numeric_limits<float>::max();
        for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {

          float fDist = sqrt( float(vfP.x-currentSwatch.pPoints[j].x)*float(vfP.x-currentSwatch.pPoints[j].x)
                           +  float(vfP.y-currentSwatch.pPoints[j].y)*float(vfP.y-currentSwatch.pPoints[j].y) );

          if (fMinDist > fDist) {
            fMinDist = fDist;
            m_iPointSelIndex = int(j);
            m_iGradSelIndex = -1;
          }
        }

        // find closest gradient coord
        for (size_t j = 0;j<2;j++) {
          float fDist = sqrt( float(vfP.x-currentSwatch.pGradientCoords[j].x)*float(vfP.x-currentSwatch.pGradientCoords[j].x)
                           +  float(vfP.y-currentSwatch.pGradientCoords[j].y)*float(vfP.y-currentSwatch.pGradientCoords[j].y) );

          if (fMinDist > fDist) {
            fMinDist = fDist;
            m_iPointSelIndex = -1;
            m_iGradSelIndex = int(j);
          }
        }

      }

      // right mouse removes / adds points
      if (mouseButton == Qt::RightButton) {

        FLOATVECTOR2 vfP = Screen2Normalized(m_vMousePressPos);

        // find closest edge and compute the point on that edge
        float fMinDist = std::numeric_limits<float>::max();
        FLOATVECTOR2 vfInserCoord;
        int iInsertIndex = -1;

        for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
          FLOATVECTOR2 A = currentSwatch.pPoints[j];
          FLOATVECTOR2 B = currentSwatch.pPoints[(j+1)%currentSwatch.pPoints.size()];

          // check if we are deleting a point
          if (currentSwatch.pPoints.size() > 3) {
            INTVECTOR2 vPixelDist = Normalized2Offscreen(vfP)-Normalized2Offscreen(A);
            if ( sqrt( float(vPixelDist.x*vPixelDist.x+vPixelDist.y*vPixelDist.y)) <= m_iSwatchBorderSize*3) {
              ss->cexec(m_trans.fqName() + ".swatchErasePoint", 
                        static_cast<size_t>(m_iActiveSwatchIndex),
                        static_cast<size_t>(j));
              iInsertIndex = -1;
              emit SwatchChange();
              break;
            }
          }


          FLOATVECTOR2 C = vfP - A;    // Vector from a to Point
          float d = (B - A).length();    // Length of the line segment
          FLOATVECTOR2 V = (B - A)/d;    // Unit Vector from A to B
          float t = V^C;          // Intersection point Distance from A

          float fDist;
          if (t >= 0 && t <= d)
            fDist = (vfP-(A + V*t)).length();
          else
            fDist = std::numeric_limits<float>::max();


          if (fDist < fMinDist) {
            fMinDist = fDist;
            vfInserCoord = vfP;
            iInsertIndex = int(j+1);
          }

        }

        if (iInsertIndex >= 0) {
          ss->cexec(m_trans.fqName() + ".swatchInsertPoint",
                    static_cast<size_t>(m_iActiveSwatchIndex),
                    static_cast<size_t>(iInsertIndex),
                    vfInserCoord);
          emit SwatchChange();
        }
      }
      update();
    }
  } else {
    m_eSimpleDragMode = SDM_NONE;
    if (mouseButton != Qt::LeftButton) return;

    int iPrevIndex = m_iActiveSwatchIndex;

    FLOATVECTOR2 vfP = Screen2Normalized(m_vMousePressPos);
    m_iActiveSwatchIndex = PickVertex(vfP, m_iSimpleDragModeSubindex);
    if (m_iActiveSwatchIndex != -1)  {
      m_eSimpleDragMode = SDM_VERTEX;
      m_bDragging = true;
    } else {
      m_iActiveSwatchIndex = PickGradient(vfP);
      if (m_iActiveSwatchIndex != -1)  {
        m_eSimpleDragMode = SDM_GRAD_CENTER;
        m_bDragging = true;
      } else {
        m_iActiveSwatchIndex = PickEdge(vfP, m_iSimpleDragModeSubindex);
        if (m_iActiveSwatchIndex != -1)  {
          m_eSimpleDragMode = SDM_EDGE;
          m_bDragging = true;
        } else {
          m_iActiveSwatchIndex = PickSwatch(vfP);
          if (m_iActiveSwatchIndex != -1)  {
            m_eSimpleDragMode = SDM_POLY;
            m_bDragging = true;
          } else {
            if (iPrevIndex != m_iActiveSwatchIndex) SwatchChange();
            return;
          }
        }
      }
    }

    emit SwatchChange();
    update();
  }
}


bool Q2DTransferFunction::PointInPolygon(const FLOATVECTOR2& point, const TFPolygon& poly) const {
  size_t  i,j=poly.pPoints.size()-1 ;
  bool oddHits=false;

  for (i=0; i<poly.pPoints.size(); i++) {
    if ((poly.pPoints[i].y<point.y && poly.pPoints[j].y>=point.y) ||
        (poly.pPoints[j].y<point.y && poly.pPoints[i].y>=point.y)) {
      if (poly.pPoints[i].x+(point.y-poly.pPoints[i].y)/
          (poly.pPoints[j].y-poly.pPoints[i].y)*
          (poly.pPoints[j].x-poly.pPoints[i].x)<point.x) {
        oddHits=!oddHits;
      }
    }
    j=i;
  }
  return oddHits;
}

int Q2DTransferFunction::PickEdge(const FLOATVECTOR2& pickPos, int& iEdgeIndex) const {
  FLOATVECTOR2 pixelPickPos = FLOATVECTOR2(Normalized2Screen(pickPos));

  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  for (vector<TFPolygon>::const_iterator it = swatches->begin(); 
       it != swatches->end(); ++it) {
    const TFPolygon& currentSwatch = *it;

    for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
      FLOATVECTOR2 A = FLOATVECTOR2(Normalized2Screen(currentSwatch.pPoints[j]));
      FLOATVECTOR2 B = FLOATVECTOR2(Normalized2Screen(currentSwatch.pPoints[(j+1)%currentSwatch.pPoints.size()]));

      FLOATVECTOR2 C = pixelPickPos - A;  // Vector from a to Point
      float d = (B - A).length();    // Length of the line segment
      FLOATVECTOR2 V = (B - A)/d;    // Unit Vector from A to B
      float t = V^C;                 // Intersection point Distance from A

      float fDist;
      if (t >= 0 && t <= d)
        fDist = (pixelPickPos-(A + V*t)).length();
      else
        fDist = std::numeric_limits<float>::max();

      if (fDist <= max<float>(m_iSwatchBorderSize,4.0f)) {  // give the user at least four pixel to pick
        iEdgeIndex = int(j);
        return static_cast<int>(std::distance(swatches->begin(), it));
      }
    }

  }
  return -1;
}

int Q2DTransferFunction::PickGradient(const FLOATVECTOR2& pickPos) const {
  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  for (vector<TFPolygon>::const_iterator it = swatches->begin(); 
       it != swatches->end(); ++it) {
    const TFPolygon& currentSwatch = *it;

    // only consider 3 stop gradients
    if (currentSwatch.pGradientStops.size() != 3) continue;

    FLOATVECTOR2 A = currentSwatch.pGradientCoords[0]*(1-currentSwatch.pGradientStops[1].first)+
                     currentSwatch.pGradientCoords[1]*currentSwatch.pGradientStops[1].first;
    INTVECTOR2 vPixelDist = Normalized2Screen(pickPos)-Normalized2Screen(A);
    if ( sqrt( float(vPixelDist.x*vPixelDist.x+vPixelDist.y*vPixelDist.y)) <= m_iSwatchBorderSize*3) {
      return static_cast<int>(std::distance(swatches->begin(), it));
    }
  }
  return -1;
}

int Q2DTransferFunction::PickVertex(const FLOATVECTOR2& pickPos, int& iVertexIndex) const {
  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  for (vector<TFPolygon>::const_iterator it = swatches->begin(); 
       it != swatches->end(); ++it) {
    const TFPolygon& currentSwatch = *it;

    for (vector<FLOATVECTOR2>::const_iterator itPnt = currentSwatch.pPoints.begin();
         itPnt != currentSwatch.pPoints.end(); ++itPnt) {
      FLOATVECTOR2 A = *itPnt;
      INTVECTOR2 vPixelDist = Normalized2Screen(pickPos)-Normalized2Screen(A);
      if ( sqrt( float(vPixelDist.x*vPixelDist.x+vPixelDist.y*vPixelDist.y)) <= m_iSwatchBorderSize*3) {
        iVertexIndex = static_cast<int>(
            std::distance(currentSwatch.pPoints.begin(), itPnt));
        return static_cast<int>(std::distance(swatches->begin(), it));
      }
    }
  }
  return -1;
}

int Q2DTransferFunction::PickSwatch(const FLOATVECTOR2& pickPos) const {
  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  for (vector<TFPolygon>::const_iterator it = swatches->begin(); 
       it != swatches->end(); ++it) {
    const TFPolygon& currentSwatch = *it;
    if (PointInPolygon(pickPos, currentSwatch)) {
      return static_cast<int>(std::distance(swatches->begin(), it));
    }
  }
  return -1;
}

void Q2DTransferFunction::mousePressEvent(QMouseEvent *event) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;
  // call superclass method
  QWidget::mousePressEvent(event);

  // middle mouse button drags entire view
  if (event->button() == Qt::MidButton) {
    m_vMousePressPos = INTVECTOR2(event->x(), event->y());
    m_eDragMode = DRM_MOVE_ZOOM;
    return;
  }

  SetDragMode( event->modifiers() & Qt::ShiftModifier,
               event->modifiers() & Qt::ControlModifier);

  INTVECTOR2 vMousePressPos = INTVECTOR2(event->x(), event->y());
  DragInit(vMousePressPos, event->button());
}

void Q2DTransferFunction::wheelEvent(QWheelEvent *event) {
  float fZoom = 1.0f-event->delta()/5000.0f;

  FLOATVECTOR2 vNewSize(std::min(1.0f,m_vZoomWindow.z*fZoom),
                        std::min(1.0f,m_vZoomWindow.w*fZoom));

  m_vZoomWindow.x += (m_vZoomWindow.z-vNewSize.x)/2.0f;
  m_vZoomWindow.y += (m_vZoomWindow.w-vNewSize.y)/2.0f;

  m_vZoomWindow.z = vNewSize.x;
  m_vZoomWindow.w = vNewSize.y;

  if (m_vZoomWindow.x + m_vZoomWindow.z > 1.0f) m_vZoomWindow.x = 1.0f-m_vZoomWindow.z;
  if (m_vZoomWindow.y + m_vZoomWindow.w > 1.0f) m_vZoomWindow.y = 1.0f-m_vZoomWindow.w;
  if (m_vZoomWindow.x < 0) m_vZoomWindow.x = 0;
  if (m_vZoomWindow.y < 0) m_vZoomWindow.y = 0;


  m_bBackdropCacheUptodate = false;
  repaint();
}


void Q2DTransferFunction::mouseReleaseEvent(QMouseEvent *event) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;
  // call superclass method
  QWidget::mouseReleaseEvent(event);

  m_bDragging = false;
  m_bDraggingAll = false;
  m_iPointSelIndex = -1;
  m_iGradSelIndex = -1;
  m_eDragMode = DRM_NONE;
  m_mouseButton = Qt::NoButton;

  update();

  // send message to update the GLtexture
  if(m_eExecutionMode == ONRELEASE) {
    ApplyFunction();
  }
}

void Q2DTransferFunction::ApplyFunction() {
  // send message to update the GLtexture
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
}


FLOATVECTOR2 Q2DTransferFunction::Rotate(FLOATVECTOR2 point, float angle, FLOATVECTOR2 center, FLOATVECTOR2 rescale) {
  FLOATVECTOR2 temp, newpoint;
  temp = (point - center)*rescale;
  newpoint.x= temp.x*cos(angle)-temp.y*sin(angle);
  newpoint.y= temp.x*sin(angle)+temp.y*cos(angle);
  return (newpoint/rescale)+center;
}

void Q2DTransferFunction::RecomputeLowerPseudoTrisPoints(TFPolygon& currentSwatch, const FLOATVECTOR2& vHandePos) {
  FLOATVECTOR2 A = currentSwatch.pPoints[1];
  FLOATVECTOR2 B = currentSwatch.pPoints[2];

  currentSwatch.pPoints[0].x = vHandePos.x+(A.x - vHandePos.x) * (currentSwatch.pPoints[0].y- vHandePos.y)/(A.y - vHandePos.y);
  currentSwatch.pPoints[3].x = vHandePos.x+(B.x - vHandePos.x) * (currentSwatch.pPoints[3].y- vHandePos.y)/(B.y - vHandePos.y);
}

void Q2DTransferFunction::mouseMoveEvent(QMouseEvent *event) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(ss) == false) return;
  // call superclass method
  QWidget::mouseMoveEvent(event);

  if (m_eDragMode == DRM_MOVE_ZOOM) {
    INTVECTOR2 vMouseCurrentPos(event->x(), event->y());

    FLOATVECTOR2 vfPressPos = Screen2Normalized(m_vMousePressPos);
    FLOATVECTOR2 vfCurrentPos = Screen2Normalized(vMouseCurrentPos);

    FLOATVECTOR2 vfDelta = vfPressPos-vfCurrentPos;

    FLOATVECTOR2 vfWinShift;

    if (vfDelta.x < 0)
      vfWinShift.x = std::max(vfDelta.x, -m_vZoomWindow.x);
    else
      vfWinShift.x = std::min(vfDelta.x, 1.0f-(m_vZoomWindow.x+m_vZoomWindow.z));

    if (vfDelta.y < 0)
      vfWinShift.y = std::max(vfDelta.y, -m_vZoomWindow.y);
    else
      vfWinShift.y = std::min(vfDelta.y, 1.0f-(m_vZoomWindow.y+m_vZoomWindow.w));

    m_vZoomWindow.x += vfWinShift.x;
    m_vZoomWindow.y += vfWinShift.y;
    m_bBackdropCacheUptodate = false;

    m_vMousePressPos = vMouseCurrentPos;
    update();

    return;
  }

  if (m_bDragging) {

    INTVECTOR2 vMouseCurrentPos(event->x(), event->y());

    FLOATVECTOR2 vfPressPos = Screen2Normalized(m_vMousePressPos);
    FLOATVECTOR2 vfCurrentPos = Screen2Normalized(vMouseCurrentPos);
    FLOATVECTOR2 vfDelta = vfCurrentPos-vfPressPos;

    if (m_eTransferFunctionMode == TFM_EXPERT) {

      // current swatch will be used as a staging area for the Lua scripting
      // system. When we are done modifying the current swatch, we will call
      // swatchUpdate. This will allow us to record the update for provenance 
      // purposes.
      shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
      TFPolygon currentSwatch = (*swatches)[m_iActiveSwatchIndex];

      if (m_bDraggingAll)  {
        switch (m_eDragMode) {
          case DRM_MOVE : {
                    for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++) currentSwatch.pPoints[i] += vfDelta;
                    currentSwatch.pGradientCoords[0] += vfDelta;
                    currentSwatch.pGradientCoords[1] += vfDelta;
                  } break;
          case DRM_ROTATE : {
                    float fScaleFactor = vfDelta.x + vfDelta.y;
                    FLOATVECTOR2 vfCenter(0,0);
                    for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++) vfCenter += currentSwatch.pPoints[i];

                    vfCenter /= currentSwatch.pPoints.size();
                    FLOATVECTOR2 fvRot(cos(fScaleFactor/10),sin(fScaleFactor/10));

                    FLOATVECTOR2 vfRescale(width(),height());
                    vfRescale /= max(width(),height());

                    for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++) currentSwatch.pPoints[i] = Rotate(currentSwatch.pPoints[i], fScaleFactor, vfCenter, vfRescale);
                    currentSwatch.pGradientCoords[0] = Rotate(currentSwatch.pGradientCoords[0], fScaleFactor, vfCenter, vfRescale);
                    currentSwatch.pGradientCoords[1] = Rotate(currentSwatch.pGradientCoords[1], fScaleFactor, vfCenter, vfRescale);


                    } break;
          case DRM_SCALE : {
                    float fScaleFactor = vfDelta.x + vfDelta.y;
                    FLOATVECTOR2 vfCenter(0,0);
                    for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++) vfCenter += currentSwatch.pPoints[i];

                    vfCenter /= currentSwatch.pPoints.size();

                    for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++)
                      currentSwatch.pPoints[i] += (currentSwatch.pPoints[i]-vfCenter)*fScaleFactor;
                    currentSwatch.pGradientCoords[0] += (currentSwatch.pGradientCoords[0]-vfCenter)*fScaleFactor;
                    currentSwatch.pGradientCoords[1] += (currentSwatch.pGradientCoords[1]-vfCenter)*fScaleFactor;

                    } break;
          default : break;
        }
      } else {
        if (m_iPointSelIndex >= 0)  {
          currentSwatch.pPoints[m_iPointSelIndex] += vfDelta;
        } else {
          currentSwatch.pGradientCoords[m_iGradSelIndex] += vfDelta;
        }
      }

      ss->cexec(m_trans.fqName() + ".swatchUpdate", 
                static_cast<size_t>(m_iActiveSwatchIndex), currentSwatch);
    } else {
      if (m_iActiveSwatchIndex < 0) return;
      shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
      TFPolygon currentSwatch = (*swatches)[m_iActiveSwatchIndex];

      switch (m_eSimpleDragMode) {
        case SDM_NONE:
          T_ERROR("No drag mode configured!");
          break;
        case SDM_GRAD_CENTER : {

          float prevCenterPosX = (currentSwatch.pGradientCoords[0]*(1-currentSwatch.pGradientStops[1].first)+
                                  currentSwatch.pGradientCoords[1]*currentSwatch.pGradientStops[1].first).x;

          currentSwatch.pGradientStops[1].first = (currentSwatch.pGradientCoords[0].x-(prevCenterPosX+vfDelta.x))/
                                                  (currentSwatch.pGradientCoords[0].x-currentSwatch.pGradientCoords[1].x);

          currentSwatch.pGradientStops[1].first = max(currentSwatch.pGradientStops[0].first, currentSwatch.pGradientStops[1].first);
          currentSwatch.pGradientStops[1].first = min(currentSwatch.pGradientStops[2].first-0.000001f, currentSwatch.pGradientStops[1].first);
        } break;
        case SDM_POLY : {
          if (m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_eType == PT_PSEUDOTRIS) vfDelta.y = 0.0;
          for (unsigned int i= 0;i<currentSwatch.pPoints.size();i++) currentSwatch.pPoints[i] += vfDelta;
          currentSwatch.pGradientCoords[0] += vfDelta;
          currentSwatch.pGradientCoords[1] += vfDelta;
          UpdateSwatchType(m_iActiveSwatchIndex);
        } break;
        case SDM_EDGE : {
          if (m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_eType == PT_OTHER) {
            currentSwatch.pPoints[m_iSimpleDragModeSubindex] += vfDelta;
            currentSwatch.pPoints[(m_iSimpleDragModeSubindex+1)%currentSwatch.pPoints.size()] += vfDelta;
            UpdateSwatchType(m_iActiveSwatchIndex);
          } else if (m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_eType == PT_RECTANGLE) {
            switch (m_iSimpleDragModeSubindex) {
              case 0 : currentSwatch.pPoints[0].x += vfDelta.x;
                       currentSwatch.pPoints[1].x = currentSwatch.pPoints[0].x;
                       break;
              case 1 : currentSwatch.pPoints[1].y += vfDelta.y;
                       currentSwatch.pPoints[2].y = currentSwatch.pPoints[1].y;
                       break;
              case 2 : currentSwatch.pPoints[2].x += vfDelta.x;
                       currentSwatch.pPoints[3].x = currentSwatch.pPoints[2].x;
                       break;
              case 3 : currentSwatch.pPoints[3].y += vfDelta.y;
                       currentSwatch.pPoints[0].y = currentSwatch.pPoints[3].y;
                       break;
            }
            currentSwatch.pGradientCoords[0] = FLOATVECTOR2(currentSwatch.pPoints[1].x, (currentSwatch.pPoints[0].y+currentSwatch.pPoints[1].y)/2.0f);
            currentSwatch.pGradientCoords[1] = FLOATVECTOR2(currentSwatch.pPoints[2].x, (currentSwatch.pPoints[2].y+currentSwatch.pPoints[3].y)/2.0f);

          } else { // pseudo triangle

            switch (m_iSimpleDragModeSubindex) {
              case 1 : currentSwatch.pPoints[1] += vfDelta;
                       currentSwatch.pPoints[2] += vfDelta;
                       break;
              case 3 : currentSwatch.pPoints[0].y += vfDelta.y;
                       currentSwatch.pPoints[3].y += vfDelta.y;
                       break;
            }

            // make sure the lower edge (which happens to be close to 1 as the coordinate system has the
            // origin in the upper left corner) of the pseudo triangle is not shrunk to a point
            if (currentSwatch.pPoints[0].y > 0.95f) {
              currentSwatch.pPoints[0].y = 0.95f;
              currentSwatch.pPoints[3].y = 0.95f;
            }

            // user dragged the top line under the botom line -> swap lines
            if (currentSwatch.pPoints[1].y > currentSwatch.pPoints[0].y) {
              std::swap(currentSwatch.pPoints[0],currentSwatch.pPoints[1]);
              std::swap(currentSwatch.pPoints[2],currentSwatch.pPoints[3]);
            }

            RecomputeLowerPseudoTrisPoints(currentSwatch, m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_vHandlePos);
            ComputeGradientForPseudoTris(currentSwatch, currentSwatch.pGradientStops[1].second);
          }
        } break;
        case SDM_VERTEX : {
          if (m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_eType == PT_OTHER) {
            currentSwatch.pPoints[m_iSimpleDragModeSubindex] += vfDelta;
            UpdateSwatchType(m_iActiveSwatchIndex);
          } else
            if (m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_eType == PT_RECTANGLE) {
            currentSwatch.pPoints[m_iSimpleDragModeSubindex] += vfDelta;

            switch (m_iSimpleDragModeSubindex) {
              case 0 : currentSwatch.pPoints[1].x = currentSwatch.pPoints[0].x;
                       currentSwatch.pPoints[3].y = currentSwatch.pPoints[0].y;
                       break;
              case 1 : currentSwatch.pPoints[0].x = currentSwatch.pPoints[1].x;
                       currentSwatch.pPoints[2].y = currentSwatch.pPoints[1].y;
                       break;
              case 2 : currentSwatch.pPoints[3].x = currentSwatch.pPoints[2].x;
                       currentSwatch.pPoints[1].y = currentSwatch.pPoints[2].y;
                       break;
              case 3 : currentSwatch.pPoints[2].x = currentSwatch.pPoints[3].x;
                       currentSwatch.pPoints[0].y = currentSwatch.pPoints[3].y;
                       break;
            }
            currentSwatch.pGradientCoords[0] = FLOATVECTOR2(currentSwatch.pPoints[1].x, (currentSwatch.pPoints[0].y+currentSwatch.pPoints[1].y)/2.0f);
            currentSwatch.pGradientCoords[1] = FLOATVECTOR2(currentSwatch.pPoints[2].x, (currentSwatch.pPoints[2].y+currentSwatch.pPoints[3].y)/2.0f);
          } else {
            // must be PT_PSEUDOTRIS

            if (m_iSimpleDragModeSubindex == 1 || m_iSimpleDragModeSubindex == 2){
              currentSwatch.pPoints[m_iSimpleDragModeSubindex] += vfDelta;

              switch (m_iSimpleDragModeSubindex) {
                case 1 : currentSwatch.pPoints[2].y = currentSwatch.pPoints[1].y;
                         break;
                case 2 : currentSwatch.pPoints[1].y = currentSwatch.pPoints[2].y;
                         break;
              }

              // user dragged the top line under the botom line -> swap lines
              if (currentSwatch.pPoints[1].y > currentSwatch.pPoints[0].y) {
                std::swap(currentSwatch.pPoints[0],currentSwatch.pPoints[1]);
                std::swap(currentSwatch.pPoints[2],currentSwatch.pPoints[3]);
              }

              RecomputeLowerPseudoTrisPoints(currentSwatch, m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_vHandlePos);
              ComputeGradientForPseudoTris(currentSwatch, currentSwatch.pGradientStops[1].second);
            }
          }
        } break;
      }

      ss->cexec(m_trans.fqName() + ".swatchUpdate", 
                static_cast<size_t>(m_iActiveSwatchIndex), currentSwatch);
    }

    m_vMousePressPos = vMouseCurrentPos;

    update();

    // send message to update the GLtexture
    if( m_eExecutionMode == CONTINUOUS ) ApplyFunction();
  }
}

void Q2DTransferFunction::SetColor(bool bIsEnabled) {
  if (bIsEnabled) {
    m_colorHistogram = QColor(255,255,255);
    m_colorBack = QColor(Qt::black);
    m_colorSwatchBorder = QColor(180, 0, 0);
    m_colorSwatchBorderHighlight = QColor(255, 190, 190);
    m_colorSwatchBorderHighlightCenter = QColor(255, 255, 255);
    m_colorSwatchBorderInactive = QColor(50, 50, 50);
    m_colorSwatchBorderInactiveHighlight = QColor(120, 120, 120);
    m_colorSwatchBorderCircle = QColor(200, 200, 0);
    m_colorSwatchBorderCircleHighlight = QColor(255, 255, 0);
    m_colorSwatchGradCircle = QColor(0, 255, 0);
    m_colorSwatchGradCircleSel = QColor(255, 255, 255);
    m_colorSwatchBorderCircleSel = QColor(255, 255, 255);
  } else {
    m_colorHistogram = QColor(55,55,55);
    m_colorBack = QColor(Qt::black);
    m_colorSwatchBorder = QColor(100, 50, 50);
    m_colorSwatchBorderHighlight = QColor(150, 70, 70);
    m_colorSwatchBorderHighlightCenter = QColor(100, 100, 100);
    m_colorSwatchBorderInactive = QColor(50, 50, 50);
    m_colorSwatchBorderInactiveHighlight = QColor(70, 70, 70);
    m_colorSwatchBorderCircle = QColor(100, 100, 50);
    m_colorSwatchBorderCircleHighlight = QColor(200, 200, 60);
    m_colorSwatchGradCircle = QColor(50, 100, 50);
    m_colorSwatchGradCircleSel = m_colorSwatchGradCircle;
    m_colorSwatchBorderCircleSel = m_colorSwatchBorderCircle;
  }
  m_bHistogramChanged = true;
}

void Q2DTransferFunction::resizeEvent ( QResizeEvent * event ) {
  QTransferFunction::resizeEvent(event);

  m_bBackdropCacheUptodate = false;
}

void Q2DTransferFunction::changeEvent(QEvent * event) {
  // call superclass method
  QWidget::changeEvent(event);

  if (event->type() == QEvent::EnabledChange) {
    SetColor(isEnabled());
    m_bBackdropCacheUptodate = false;
    update();
  }
}

void Q2DTransferFunction::ClearToBlack(QPainter& painter) {
  painter.setPen(Qt::NoPen);
  painter.setBrush(m_colorBack);
  QRect backRect(0,0,painter.viewport().width(),painter.viewport().height());
  painter.drawRect(backRect);}


void Q2DTransferFunction::Draw1DTrans(QPainter& painter) {
  QRectF imageRect(0, 0,
                  painter.viewport().width(), painter.viewport().height());

  // This hack exists because IO has a dependency on QT.
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  LuaTransferFun2DProxy* prox = m_trans.getRawPointer<LuaTransferFun2DProxy>(ss);
  TransferFunction2D* hackTF2D = prox->get2DTransferFunction();
  if (hackTF2D == NULL) return;

  QRectF source(m_vZoomWindow.x * hackTF2D->Get1DTransImage().width(),
                m_vZoomWindow.y * hackTF2D->Get1DTransImage().height(),
                m_vZoomWindow.z * hackTF2D->Get1DTransImage().width(),
                m_vZoomWindow.w * hackTF2D->Get1DTransImage().height());
  painter.drawImage(imageRect,hackTF2D->Get1DTransImage(), source);
}


void
Q2DTransferFunction::ComputeCachedImageSize(uint32_t &w , uint32_t &h) const {
  // find an image size that has the same aspect ratio as the histogram
  // but is no smaller than the widget

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  VECTOR2<size_t> rendererSize = ss->cexecRet<VECTOR2<size_t>>(
      m_trans.fqName() + ".getRenderSize");

  w = uint32_t(width());
  float fRatio = float(rendererSize.x) / float(rendererSize.y);
  h = static_cast<uint32_t>(w / fRatio);

  if (h > uint32_t(height())) {
    h = uint32_t(height());
    w = static_cast<uint32_t>(h * fRatio);
  }
}

void Q2DTransferFunction::paintEvent(QPaintEvent *event) {
  // call superclass method
  QWidget::paintEvent(event);

  if (m_trans.isValid(m_MasterController.LuaScript()) == false) {
    QPainter painter(this);
    ClearToBlack(painter);
    return;
  }

  uint32_t w,h;
  ComputeCachedImageSize(w,h);

  // as drawing the histogram can become quite expensive we'll cache it in an image and only redraw if needed
  if (m_bHistogramChanged || !m_bBackdropCacheUptodate || h != m_iCachedHeight || w != m_iCachedWidth) {

    delete m_pBackdropCache;
    m_pBackdropCache = new QPixmap(w,h);

    // attach a painter to the pixmap
    QPainter image_painter(m_pBackdropCache);

    // draw the backdrop into the image
    DrawHistogram(image_painter);
    Draw1DTrans(image_painter);

    // update change detection states
    m_bBackdropCacheUptodate = true;
    m_iCachedHeight = h;
    m_iCachedWidth = w;
  }

  // now draw everything rest into this widget
  QPainter painter;

  QPixmap tmp(w, h);
  painter.begin(&tmp);


  //painter.eraseRect(0,0,w,h);

  // the image captured before (or cached from a previous call)
  painter.drawImage(0,0,m_pBackdropCache->toImage());

  // and the swatches
  DrawSwatches(painter);

  painter.end();
  painter.begin(this);

  QRectF source(0.0, 0.0, w, h);
  QRectF target(0.0, 0.0, width(), height());

  painter.drawImage(target, tmp.toImage(), source);
  DrawSwatcheDecoration(painter);

  painter.end();
}

bool Q2DTransferFunction::LoadFromFile(const QString& strFilename) {
  // hand the load call over to the TransferFunction1D class
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  VECTOR2<size_t> transSize = ss->cexecRet<VECTOR2<size_t>>(m_trans.fqName() +
                                                            ".getSize");
  if (ss->cexecRet<bool>(m_trans.fqName() + ".loadWithSize", 
                         strFilename.toStdWString(), transSize)) {
    m_iActiveSwatchIndex = 0;
    m_bBackdropCacheUptodate = false;
    update();
    ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
    emit SwatchChange();
    return true;
  } else return false;
}

bool Q2DTransferFunction::SaveToFile(const QString& strFilename) {
  // hand the save call over to the TransferFunction1D class
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  return ss->cexecRet<bool>(m_trans.fqName() + ".save", 
                            strFilename.toStdWString());
}


void Q2DTransferFunction::Set1DTrans(LuaClassInstance inst) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(m_trans.fqName() + ".update1DTrans", inst);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  m_bBackdropCacheUptodate = false;
  update();
}

void Q2DTransferFunction::Transfer2DSetActiveSwatch(const int iActiveSwatch) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  if (iActiveSwatch == -1 && m_trans.isValid(ss) && 
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount") > 0) return;
  m_iActiveSwatchIndex = iActiveSwatch;
  update();
}

void Q2DTransferFunction::Transfer2DAddCircleSwatch() {
  TFPolygon newSwatch;

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());

  FLOATVECTOR2 vPoint(m_vZoomWindow.x + 0.8f*m_vZoomWindow.z, m_vZoomWindow.y + 0.8f*m_vZoomWindow.w);
  FLOATVECTOR2 vCenter(m_vZoomWindow.x + 0.5f*m_vZoomWindow.z, m_vZoomWindow.y + 0.5f*m_vZoomWindow.w);
  unsigned int iNumberOfSegments = 20;
  for (unsigned int i = 0;i<iNumberOfSegments;i++) {
    newSwatch.pPoints.push_back(vPoint);
    vPoint = Rotate(vPoint, 6.283185f/float(iNumberOfSegments), vCenter, FLOATVECTOR2(1,1));
  }

  newSwatch.pGradientCoords[0] = FLOATVECTOR2(m_vZoomWindow.x + 0.1f*m_vZoomWindow.z, m_vZoomWindow.y + 0.5f*m_vZoomWindow.w);
  newSwatch.pGradientCoords[1] = FLOATVECTOR2(m_vZoomWindow.x + 0.9f*m_vZoomWindow.z, m_vZoomWindow.y + 0.5f*m_vZoomWindow.w);

  GradientStop g1(0.0f, FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f)),
               g2(0.5f, FLOATVECTOR4(1.0f,1.0f,1.0f,1.0f)),
               g3(1.0f, FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f));
  newSwatch.pGradientStops.push_back(g1);
  newSwatch.pGradientStops.push_back(g2);
  newSwatch.pGradientStops.push_back(g3);

  ss->cexec(m_trans.fqName() + ".swatchPushBack", newSwatch);

  m_iActiveSwatchIndex = static_cast<int>(
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  emit SwatchChange();
}

void Q2DTransferFunction::Transfer2DAddSwatch() {
  TFPolygon newSwatch;

  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.7f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.7f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w));

  newSwatch.pGradientCoords[0] = FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.5f*m_vZoomWindow.w);
  newSwatch.pGradientCoords[1] = FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.5f*m_vZoomWindow.w);

  GradientStop g1(0.0f,FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f)),
               g2(0.5f,FLOATVECTOR4(1.0f,1.0f,1.0f,1.0f)),
               g3(1.0f,FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f));
  newSwatch.pGradientStops.push_back(g1);
  newSwatch.pGradientStops.push_back(g2);
  newSwatch.pGradientStops.push_back(g3);

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_trans.fqName() + ".swatchPushBack", newSwatch);

  m_iActiveSwatchIndex = static_cast<int>(
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  emit SwatchChange();
}

void Q2DTransferFunction::Transfer2DAddRectangleSwatch() {
  TFPolygon newSwatch;

  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.7f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.7f*m_vZoomWindow.w));
  newSwatch.pPoints.push_back(FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w));

  newSwatch.pGradientCoords[0] = (newSwatch.pPoints[0]+newSwatch.pPoints[1])/2.0f;
  newSwatch.pGradientCoords[1] = (newSwatch.pPoints[2]+newSwatch.pPoints[3])/2.0f;

  GradientStop g1(0.0f, FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f)),
               g2(0.5f,
                  FLOATVECTOR4(rand()/float(RAND_MAX), rand()/float(RAND_MAX),
                               rand()/float(RAND_MAX), 1.0f)),
               g3(1.0f, FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f));
  newSwatch.pGradientStops.push_back(g1);
  newSwatch.pGradientStops.push_back(g2);
  newSwatch.pGradientStops.push_back(g3);

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(m_trans.fqName() + ".swatchPushBack", newSwatch);
  UpdateSwatchTypes();

  m_iActiveSwatchIndex = static_cast<int>(
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  emit SwatchChange();
}


void Q2DTransferFunction::ComputeGradientForPseudoTris(TFPolygon& swatch, const FLOATVECTOR4& color) {
  GradientStop g1(0.0f,FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f)),
               g2(0.5f,color),
               g3(1.0f,FLOATVECTOR4(0.0f,0.0f,0.0f,0.0f));
  swatch.pGradientStops.clear();
  swatch.pGradientStops.push_back(g1);
  swatch.pGradientStops.push_back(g2);
  swatch.pGradientStops.push_back(g3);

  FLOATVECTOR2 persCorrection(m_iCachedWidth,m_iCachedHeight);
  persCorrection /= persCorrection.maxVal();

  FLOATVECTOR2 vTop       = (swatch.pPoints[1]+swatch.pPoints[2])*persCorrection/2.0f;
  FLOATVECTOR2 vBottom    = (swatch.pPoints[3]+swatch.pPoints[0])*persCorrection/2.0f;
  FLOATVECTOR2 vDirection = (vBottom-vTop);
  FLOATVECTOR2 vPerpendicular(vDirection.y,-vDirection.x);
  vPerpendicular.normalize();
  vPerpendicular *= (swatch.pPoints[1]-swatch.pPoints[2]).length()/2.0f;

  swatch.pGradientCoords[0] = (vTop-vPerpendicular)/persCorrection;
  swatch.pGradientCoords[1] = (vTop+vPerpendicular)/persCorrection;
}

void Q2DTransferFunction::Transfer2DAddPseudoTrisSwatch() {
  TFPolygon newSwatch;

  FLOATVECTOR2 p1 = FLOATVECTOR2(m_vZoomWindow.x + 0.3f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w);
  FLOATVECTOR2 p2 = FLOATVECTOR2(m_vZoomWindow.x + 0.7f*m_vZoomWindow.z, m_vZoomWindow.y + 0.3f*m_vZoomWindow.w);

  FLOATVECTOR2 handlePoint = FLOATVECTOR2((p2.x+p1.x)/2.0f,1.0f);

  FLOATVECTOR2 p0 = (p1+handlePoint)/2.0f;
  FLOATVECTOR2 p3 = (p2+handlePoint)/2.0f;

  newSwatch.pPoints.push_back(p0);
  newSwatch.pPoints.push_back(p1);
  newSwatch.pPoints.push_back(p2);
  newSwatch.pPoints.push_back(p3);
  ComputeGradientForPseudoTris(newSwatch, FLOATVECTOR4(rand()/float(RAND_MAX),rand()/float(RAND_MAX),rand()/float(RAND_MAX),1));

  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(m_trans.fqName() + ".swatchPushBack", newSwatch);
  UpdateSwatchTypes();

  m_iActiveSwatchIndex = static_cast<int>(
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  emit SwatchChange();
}


void Q2DTransferFunction::Transfer2DDeleteSwatch(){
  if (m_iActiveSwatchIndex != -1) {
    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    ss->cexec(m_trans.fqName() + ".swatchErase", 
              static_cast<size_t>(m_iActiveSwatchIndex));

    int lastSwatchIndex = static_cast<int>(
        ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
    m_iActiveSwatchIndex = min<int>(m_iActiveSwatchIndex, lastSwatchIndex);
    ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
    emit SwatchChange();
  }
}

void Q2DTransferFunction::Transfer2DUpSwatch(){
  if (m_iActiveSwatchIndex > 0) {
    shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
    shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
    TFPolygon tmp = (*swatches)[m_iActiveSwatchIndex-1];

    ss->cexec(m_trans.fqName() + ".swatchUpdate", 
              static_cast<size_t>(m_iActiveSwatchIndex-1),
              (*swatches)[m_iActiveSwatchIndex]);
    ss->cexec(m_trans.fqName() + ".swatchUpdate",
              static_cast<size_t>(m_iActiveSwatchIndex),
              tmp);

    m_iActiveSwatchIndex--;
    ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
    emit SwatchChange();
  }
}

void Q2DTransferFunction::Transfer2DDownSwatch(){
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  int lastSwatchIndex = static_cast<int>(
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount")-1);
  if (m_iActiveSwatchIndex >= 0 && m_iActiveSwatchIndex < lastSwatchIndex) {
    shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
    TFPolygon tmp = (*swatches)[m_iActiveSwatchIndex+1];

    ss->cexec(m_trans.fqName() + ".swatchUpdate", 
              static_cast<size_t>(m_iActiveSwatchIndex+1),
              (*swatches)[m_iActiveSwatchIndex]);
    ss->cexec(m_trans.fqName() + ".swatchUpdate",
              static_cast<size_t>(m_iActiveSwatchIndex),
              tmp);

    m_iActiveSwatchIndex++;
    ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
    emit SwatchChange();
  }
}

void Q2DTransferFunction::SetActiveGradientType(bool bRadial) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  size_t swatchCount = 
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount");
  if (static_cast<size_t>(m_iActiveSwatchIndex) < swatchCount) {
    if (ss->cexecRet<bool>(m_trans.fqName() + ".swatchIsRadial", 
                           static_cast<size_t>(m_iActiveSwatchIndex)) 
        != bRadial) {
      ss->cexec(m_trans.fqName() + ".swatchSetRadial", bRadial);
      ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
      update();
    }
  }
}

void Q2DTransferFunction::AddGradient(GradientStop stop) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  const TFPolygon& currentSwatch = (*swatches)[m_iActiveSwatchIndex];
  for (auto i = currentSwatch.pGradientStops.begin();
       i < currentSwatch.pGradientStops.end(); ++i) {
    if (i->first > stop.first) {
      // Do not proceed iteration after making this Lua call. If the 
      // pGradientStops vector resized itself, our iterator is invalid.
      ss->cexec(m_trans.fqName() + ".swatchInsertGradient", 
               static_cast<size_t>(m_iActiveSwatchIndex),
               static_cast<size_t>(
                   std::distance(currentSwatch.pGradientStops.begin(), i)), 
               stop);
      // We have modified the transfer function, make sure to display the 
      // results...
      ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
      update();
      return;
    }
  }
  ss->cexec(m_trans.fqName() + ".swatchPushBackGradient",
            static_cast<size_t>(m_iActiveSwatchIndex),
            stop);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  update();
}

void Q2DTransferFunction::DeleteGradient(unsigned int i) {
  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  ss->cexec(m_trans.fqName() + ".swatchEraseGradient",
            static_cast<size_t>(m_iActiveSwatchIndex),
            static_cast<size_t>(i));
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  update();
}

void Q2DTransferFunction::SetGradient(unsigned int i, GradientStop stop) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec(m_trans.fqName() + ".swatchUpdateGradient",
            static_cast<size_t>(m_iActiveSwatchIndex),
            static_cast<size_t>(i),
            stop);
  ss->cexec("tuvok.gpu.changed2DTrans", LuaClassInstance(), m_trans);
  update();
}


void Q2DTransferFunction::UpdateSwatchType(size_t i) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  std::string desc = m_vSimpleSwatchInfo[i].m_strDesc;

  shared_ptr<const vector<TFPolygon>> swatches = GetSwatches();
  TFPolygon tmp = (*swatches)[i];
  m_vSimpleSwatchInfo[i] = ClassifySwatch(tmp);
  // Tmp might have been mutated by ClassifySwatch, so we need to inform
  // TransferFunction2D that it happened.
  ss->cexec(m_trans.fqName() + ".swatchUpdate", i, tmp);

  if (desc != m_vSimpleSwatchInfo[i].m_strDesc) emit SwatchTypeChange(int(i));
}


void Q2DTransferFunction::UpdateSwatchTypes() {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  size_t swatchCount = 
      ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount");

  m_vSimpleSwatchInfo.clear();
  m_vSimpleSwatchInfo.resize(swatchCount);
  for (size_t i = 0;i<swatchCount;i++) {
    UpdateSwatchType(i);
  }
}

void Q2DTransferFunction::Toggle2DTFMode() {
  Set2DTFMode(E2DTransferFunctionMode((int(m_eTransferFunctionMode)+1)%int(TFM_INVALID)));
}

void Q2DTransferFunction::Set2DTFMode(E2DTransferFunctionMode TransferFunctionMode) {
  m_eTransferFunctionMode = TransferFunctionMode;
  if (m_eTransferFunctionMode == TFM_BASIC) UpdateSwatchTypes();
  update();
}


std::string Q2DTransferFunction::GetSwatchDesciption() const {
  if (int(m_vSimpleSwatchInfo.size()) > m_iActiveSwatchIndex) {
    return m_vSimpleSwatchInfo[m_iActiveSwatchIndex].m_strDesc;
  } else {
    return "";
  }
}

size_t Q2DTransferFunction::GetSwatchCount() {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(ss)) {
    return ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetCount");
  } else {
    return 0;
  }
}

size_t Q2DTransferFunction::GetSwatchSize(unsigned int i) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(ss)) {
    return ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetNumPoints", 
                                static_cast<size_t>(i));
  } else {
    return 0;
  }
}

bool Q2DTransferFunction::GetActiveGradientType() {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if ( !m_trans.isValid(ss)
      || static_cast<size_t>(m_iActiveSwatchIndex) >= GetSwatchCount()) {
    return false;
  }
  return ss->cexecRet<bool>(m_trans.fqName() + ".swatchIsRadial", 
                            static_cast<size_t>(m_iActiveSwatchIndex));
}

size_t Q2DTransferFunction::GetGradientCount() {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if ( !m_trans.isValid(ss)
      || static_cast<size_t>(m_iActiveSwatchIndex) >= GetSwatchCount()) {
    return 0;
  }
 return ss->cexecRet<size_t>(m_trans.fqName() + ".swatchGetGradientCount", 
                             static_cast<size_t>(m_iActiveSwatchIndex));
}

GradientStop Q2DTransferFunction::GetGradient(unsigned int i) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if ( !m_trans.isValid(ss)
      || static_cast<size_t>(m_iActiveSwatchIndex) >= GetSwatchCount()) {
    // need to return something invalid.
    return GradientStop(0.0f, FLOATVECTOR4(0,0,0,0));
  }
  return ss->cexecRet<GradientStop>(m_trans.fqName() + ".swatchGetGradient", 
                                    static_cast<size_t>(m_iActiveSwatchIndex),
                                    static_cast<size_t>(i));
}


shared_ptr<const vector<TFPolygon>> Q2DTransferFunction::GetSwatches() const {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  shared_ptr<const vector<TFPolygon>> swatches = 
      ss->cexecRet<shared_ptr<const vector<TFPolygon>>>(m_trans.fqName() +
                                                        ".swatchGet");
  return swatches;
}
