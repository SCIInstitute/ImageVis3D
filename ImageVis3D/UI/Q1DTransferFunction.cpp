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


//!    File   : Q1DTransferFunction.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#include <iostream>

#include "Q1DTransferFunction.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/MathTools.h"
#include "../Tuvok/Renderer/GPUMemMan/GPUMemMan.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTransferFun1DProxy.h"

using namespace std;


Q1DTransferFunction::Q1DTransferFunction(MasterController& masterController, QWidget *parent) :
  QTransferFunction(masterController, parent),
  m_iPaintMode(PAINT_RED | PAINT_GREEN | PAINT_BLUE | PAINT_ALPHA),
  m_iCachedHeight(0),
  m_iCachedWidth(0),
  m_pBackdropCache(NULL),
  m_pPreviewBack(NULL),
  m_pPreviewColor(NULL),
  // borders, may be changed arbitrarily
  m_iLeftBorder(20),
  m_iBottomBorder(20),
  m_iTopPreviewHeight(30),
  m_iTopPreviewDist(10),
  // automatically computed borders
  m_iRightBorder(0),
  m_iTopBorder(0),
  // scale apearance, may be changed arbitrarily (except for the marker length wich should be less or equal to both m_iLeftBorder and m_iBottomBorder)
  m_iMarkersX(50),
  m_iMarkersY(20),
  m_iBigMarkerSpacingX(10),
  m_iBigMarkerSpacingY(5),
  m_iMarkerLength(5),
  m_iBigMarkerLength(m_iMarkerLength*3),
  // mouse motion
  m_iLastIndex(-1),
  m_fLastValue(0),
  m_bMouseLeft(false),
  m_bMouseRight(false)
{
  SetColor(isEnabled());
}

Q1DTransferFunction::~Q1DTransferFunction(void)
{
  // delete the backdrop cache pixmap
  delete m_pBackdropCache;
  delete m_pPreviewBack;
  delete m_pPreviewColor;
}

void Q1DTransferFunction::PreparePreviewData() {
  delete m_pPreviewColor;
  m_pPreviewColor = new QImage(int(m_vHistogram.GetSize()),1, QImage::Format_ARGB32);
  m_bBackdropCacheUptodate = false;
}

void Q1DTransferFunction::SetData(shared_ptr<const Histogram1D> vHistogram,
                                  unsigned int iMaxValue,
                                  LuaClassInstance trans) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  // There has got to be a better way of checking if a shared_ptr is valid...
  // We can't use nullptr yet because of our octagon ci machine.
  if (trans.isValid(ss) == false || vHistogram == 0) return;

  m_trans = trans;

  m_iMarkersX = std::max<unsigned int>(1,iMaxValue);
  while (m_iMarkersX > 100) m_iMarkersX /= 10;

  // resize internal histogram, we only consider histogram values to the maximum entry that is non-zero
  m_vHistogram.Resize(vHistogram->GetFilledSize());

  // resize the preview bar and force the draw routine to recompute the backdrop cache
  PreparePreviewData();

  // if the histogram is empty we are done
  if (m_vHistogram.GetSize() == 0)  return;

  // rescale the histogram to the [0..1] range
  // first find min and max ...
  unsigned int iMax = vHistogram->GetLinear(0);
  unsigned int iMin = iMax;
  for (size_t i = 0;i<m_vHistogram.GetSize();i++) {
    unsigned int iVal = vHistogram->GetLinear(i);
    if (iVal > iMax) iMax = iVal;
    if (iVal < iMin) iMin = iVal;
  }

  // ... than rescale
  float fDiff = float(iMax)-float(iMin);
  for (size_t i = 0;i<m_vHistogram.GetSize();i++)
    m_vHistogram.SetLinear(i, (float(vHistogram->GetLinear(i)) - float(iMin)) / fDiff);
}

void Q1DTransferFunction::DrawCoordinateSystem(QPainter& painter) {

  unsigned int iReducedLeftBorder = m_iLeftBorder - 2;

  // adjust left and bottom border so that the marker count can be met
  unsigned int iReducedRightBorder = (width()-(iReducedLeftBorder+2)) %m_iMarkersX;
  m_iTopBorder     = (height()-(m_iBottomBorder+2+m_iTopPreviewHeight+m_iTopPreviewDist)) %m_iMarkersY + m_iTopPreviewHeight+m_iTopPreviewDist;

  // compute the actual marker spaceing
  unsigned int iMarkerSpacingX = (width()-(iReducedLeftBorder+2))/m_iMarkersX;
  unsigned int iMarkerSpacingY = (height()-(m_iBottomBorder+2+m_iTopBorder))/m_iMarkersY;

  // draw background
  painter.setBrush(m_colorBack);
  QRect backRect(0,0,width(),height());
  painter.drawRect(backRect);

  // draw grid borders
  QPen borderPen(m_colorBorder, 1, Qt::SolidLine);
  painter.setPen(borderPen);
  QRect borderRect(iReducedLeftBorder, m_iTopBorder, width()-(iReducedLeftBorder+2+iReducedRightBorder), height()-(m_iTopBorder+m_iBottomBorder+2));
  painter.drawRect(borderRect);

  // draw Y axis markers
  QPen penScale(m_colorScale, 1, Qt::SolidLine);
  QPen penLargeScale(m_colorLargeScale, 1, Qt::SolidLine);
  for (unsigned int i = 0;i<m_iMarkersY;i++) {
    int iPosY = height()-m_iBottomBorder-2 - i*iMarkerSpacingY;

    if (i%m_iBigMarkerSpacingY == 0) {
      painter.setPen(penLargeScale);
      painter.drawLine(iReducedLeftBorder-m_iBigMarkerLength, iPosY, iReducedLeftBorder, iPosY);
    } else {
      painter.setPen(penScale);
      painter.drawLine(iReducedLeftBorder-m_iMarkerLength, iPosY, iReducedLeftBorder, iPosY);
    }
  }

  // draw X axis markers
  unsigned int iStartY = height()-(m_iBottomBorder+2);
  for (unsigned int i = 0;i<m_iMarkersX;i++) {
    int iPosX = iReducedLeftBorder+i*iMarkerSpacingX;
    if (i%m_iBigMarkerSpacingX == 0) {
      painter.setPen(penLargeScale);
      painter.drawLine(iPosX, iStartY+m_iBigMarkerLength, iPosX, iStartY);
    } else {
      painter.setPen(penScale);
      painter.drawLine(iPosX, iStartY+m_iMarkerLength, iPosX, iStartY);
    }
  }

  m_iRightBorder = iReducedRightBorder+2;
}

void Q1DTransferFunction::DrawHistogram(QPainter& painter) {

  if (m_trans.isValid(m_MasterController.LuaScript()) == false 
      || m_vHistogram.GetSize() < 2) return;

  // compute some grid dimensions
  unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
  unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;

  // draw the histogram a as large polygon
  // define the polygon ...
  std::vector<QPointF> pointList;
  pointList.push_back(QPointF(m_iLeftBorder+1, iGridHeight-m_iBottomBorder));

  if (iGridWidth < m_vHistogram.GetSize()) {
    float fHistStep = m_vHistogram.GetSize()/float(iGridWidth);
    size_t iHistStep = size_t(ceil(fHistStep));

    for (size_t i = 0;i<iGridWidth;i++) {
      float singleValue = 0;
      for (size_t j = 0;j<iHistStep;j++) {
        singleValue += m_vHistogram.Get(std::min(int(i*fHistStep)+j, m_vHistogram.GetSize()-1));
      }
      singleValue /= iHistStep;

      float value = min<float>(1.0f, pow(singleValue,1.0f/(1+(m_fHistfScale-1)/100.0f)));
      pointList.push_back(QPointF(m_iLeftBorder+1+i,
                                  m_iTopBorder+iGridHeight-value*iGridHeight));
    }
  } else {
    for (size_t i = 0;i<m_vHistogram.GetSize();i++) {
      float value = min<float>(1.0f, pow(m_vHistogram.Get(i),1.0f/(1+(m_fHistfScale-1)/100.0f)));
      pointList.push_back(QPointF(m_iLeftBorder+1+float(iGridWidth)*i/(m_vHistogram.GetSize()-1),
                                  m_iTopBorder+iGridHeight-value*iGridHeight));
    }
  }

  pointList.push_back(QPointF(m_iLeftBorder+iGridWidth, m_iTopBorder+iGridHeight));
  pointList.push_back(QPointF(m_iLeftBorder+1, m_iTopBorder+iGridHeight));

  // ... draw it
  painter.setPen(Qt::NoPen);
  painter.setBrush(m_colorHistogram);
  painter.drawPolygon(&pointList[0], int(pointList.size()));
}

void Q1DTransferFunction::DrawFunctionPlots(QPainter& painter) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // compute some grid dimensions
  unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
  unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;

  // draw the tranfer function as one larger polyline
  std::vector<QPointF> pointList(m_vHistogram.GetSize());
  QPen penCurve(m_colorBorder, 2, Qt::CustomDashLine);

  QVector<qreal> dashes;
  qreal space = 4;
  dashes << 1 << space;
  penCurve.setDashPattern(dashes);

  // Retrieve color data from Lua script class.
  std::shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  shared_ptr<vector<FLOATVECTOR4>> cdata = 
      ss->cexecRet<shared_ptr<vector<FLOATVECTOR4> > >(
          m_trans.fqName() + ".getColorData");

  // for every component
  for (unsigned int j=0; j < 4; j++) {
    // select the color
    switch (j) {
      case 0  : penCurve.setColor(m_colorRedLine);  
                penCurve.setDashOffset(0.0);
                break;
      case 1  : penCurve.setColor(m_colorGreenLine); 
                penCurve.setDashOffset(1.0);
                break;
      case 2  : penCurve.setColor(m_colorBlueLine);  
                penCurve.setDashOffset(2.0);
                break;
      default : penCurve.setColor(m_colorAlphaLine); 
                penCurve.setDashOffset(3.0);
                break;
    }

    // Make sure our tfqn is as wide as our histogram, since we'll be accessing
    // every element in the histogram.
    assert(cdata->size() >= m_vHistogram.GetSize());

    // define the polyline
    for (size_t i=0; i < pointList.size(); i++) {
      pointList[i] = QPointF(
          m_iLeftBorder + 1 + float(iGridWidth) * i / (pointList.size() - 1),
          m_iTopBorder + iGridHeight - std::max(0.0f,std::min((*cdata)[i][j],1.0f)) * iGridHeight
      );
    }

    // draw the polyline
    painter.setPen(penCurve);
    painter.drawPolyline(&pointList[0], int(pointList.size()));
  }


  // draw preview bar
  for (unsigned int x = 0;x<m_vHistogram.GetSize();x++) {

    float r = std::max(0.0f,std::min((*cdata)[x][0],1.0f));
    float g = std::max(0.0f,std::min((*cdata)[x][1],1.0f));
    float b = std::max(0.0f,std::min((*cdata)[x][2],1.0f));
    float a = std::max(0.0f,std::min((*cdata)[x][3],1.0f));

    m_pPreviewColor->setPixel(x,0,qRgba(int(r*255),int(g*255),int(b*255),int(a*255)));
  }

  QRect prevRect(m_iLeftBorder+1,
                 m_iTopBorder - (m_iTopPreviewHeight + m_iTopPreviewDist),
                 width() - (m_iLeftBorder + 3 + m_iRightBorder),
                 m_iTopPreviewHeight);
  painter.drawImage(prevRect,*m_pPreviewBack);
  painter.drawImage(prevRect,*m_pPreviewColor);
}

void Q1DTransferFunction::mousePressEvent(QMouseEvent *event) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // call superclass method
  QWidget::mousePressEvent(event);
  // clear the "last position" index
  m_iLastIndex = -1;

  if (event->button() == Qt::LeftButton)  m_bMouseLeft = true;
  if (event->button() == Qt::RightButton) m_bMouseRight = true;
}

void Q1DTransferFunction::mouseReleaseEvent(QMouseEvent *event) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // call superclass method
  QWidget::mouseReleaseEvent(event);
  // clear the "last position" index

  m_iLastIndex = -1;

  if (event->button() == Qt::LeftButton) m_bMouseLeft = false;
  if (event->button() == Qt::RightButton) m_bMouseRight = false;

  // send message to update the GLtexture
  if(m_eExecutionMode == ONRELEASE) {
    ApplyFunction();
  }
}

void Q1DTransferFunction::mouseMoveEvent(QMouseEvent *event) {
  if (m_trans.isValid(m_MasterController.LuaScript()) == false) return;

  // call superclass method
  QWidget::mouseMoveEvent(event);

  // exit if nothing is to be painted
  if (m_iPaintMode == PAINT_NONE) return;

  // compute some grid dimensions
  unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
  unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;
  unsigned int iVectorSize = uint32_t(m_vHistogram.GetSize());

  // compute position in color array
  int iCurrentIndex = int(0.5f+(float(event->x())-float(m_iLeftBorder)-1.0f)*float(iVectorSize-1)/float(iGridWidth));
  iCurrentIndex = std::min<int>(iVectorSize-1, std::max<int>(0,iCurrentIndex));

  // compute actual color value
  float fValue = (float(m_iTopBorder)+float(iGridHeight)-float(event->y()))/float(iGridHeight);
  fValue    = std::min<float>(1.0f, std::max<float>(0.0f,fValue));

  // Retrieve color data from Lua script class.
  std::shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  shared_ptr<vector<FLOATVECTOR4> > cdata = 
      ss->cexecRet<shared_ptr<vector<FLOATVECTOR4> > >(
          m_trans.fqName() + ".getColorData");

  if (m_bMouseLeft) {
    // find out the range to change
    if (m_iLastIndex == -1) {
      m_iLastIndex = iCurrentIndex;
      m_fLastValue = fValue;
    }

    int iIndexMin, iIndexMax;
    float fValueMin, fValueInc;

    if (m_iLastIndex < iCurrentIndex) {
      iIndexMin = m_iLastIndex;
      iIndexMax = iCurrentIndex;

      fValueMin = m_fLastValue;
      fValueInc = -(fValue-m_fLastValue)/(m_iLastIndex-iCurrentIndex);
    } else {
      iIndexMin = iCurrentIndex;
      iIndexMax = m_iLastIndex;

      fValueMin = fValue;
      fValueInc = -(fValue-m_fLastValue)/(m_iLastIndex-iCurrentIndex);
    }

    m_iLastIndex = iCurrentIndex;
    m_fLastValue = fValue;

    // update transfer function
    if (m_iPaintMode & PAINT_RED) {
      float _fValueMin = fValueMin;
      for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
        (*cdata)[iIndex][0] = _fValueMin;
        _fValueMin += fValueInc;
      }
    }
    if (m_iPaintMode & PAINT_GREEN) {
      float _fValueMin = fValueMin;
      for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
        (*cdata)[iIndex][1] = _fValueMin;
        _fValueMin += fValueInc;
      }
    }
    if (m_iPaintMode & PAINT_BLUE) {
      float _fValueMin = fValueMin;
      for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
        (*cdata)[iIndex][2] = _fValueMin;
        _fValueMin += fValueInc;
      }
    }
    if (m_iPaintMode & PAINT_ALPHA) {
      float _fValueMin = fValueMin;
      for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
        (*cdata)[iIndex][3] = _fValueMin;
        _fValueMin += fValueInc;
      }
    }

    // redraw this widget
    update();
    // send message to update the GLtexture
    if( m_eExecutionMode == CONTINUOUS ) ApplyFunction();
  } else {
    if (m_bMouseRight) {

      bool bShiftPressed = ( event->modifiers() & Qt::ShiftModifier);

      // set "step" function
      size_t cdataSize = cdata->size();
      if (m_iPaintMode & PAINT_RED)   
        ss->cexec(m_trans.fqName() + ".setStdFunction", 
                  float(iCurrentIndex)/float(cdataSize), fValue, 
                  0, bShiftPressed);
      if (m_iPaintMode & PAINT_GREEN)
        ss->cexec(m_trans.fqName() + ".setStdFunction", 
                  float(iCurrentIndex)/float(cdataSize), fValue, 
                  1, bShiftPressed);
      if (m_iPaintMode & PAINT_BLUE)
        ss->cexec(m_trans.fqName() + ".setStdFunction", 
                  float(iCurrentIndex)/float(cdataSize), fValue, 
                  2, bShiftPressed);
      if (m_iPaintMode & PAINT_ALPHA)
        ss->cexec(m_trans.fqName() + ".setStdFunction", 
                  float(iCurrentIndex)/float(cdataSize), fValue, 
                  3, bShiftPressed);

      // redraw this widget
      update();
      // send message to update the GLtexture
      if( m_eExecutionMode == CONTINUOUS ) ApplyFunction();
    }
  }
}

void Q1DTransferFunction::ApplyFunction() {
  // send message to update the GLtexture
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  ss->cexec("tuvok.gpu.changed1DTrans", LuaClassInstance(), m_trans);
}

void Q1DTransferFunction::SetColor(bool bIsEnabled) {
  if (bIsEnabled) {
    m_colorHistogram = QColor(50,50,50);
    m_colorBack = QColor(Qt::black);
    m_colorBorder = QColor(100, 100, 255);
    m_colorScale = QColor(100, 100, 255);
    m_colorLargeScale = QColor(180, 180, 180);
    m_colorRedLine = QColor(255, 0, 0);
    m_colorGreenLine = QColor(0, 255, 0);
    m_colorBlueLine = QColor(0, 0, 255);
    m_colorAlphaLine = QColor(Qt::white);
  } else {
    m_colorHistogram = QColor(50,50,50);
    m_colorBack = QColor(Qt::black);
    m_colorBorder = QColor(100, 100, 100);
    m_colorScale = QColor(100, 100, 100);
    m_colorLargeScale = QColor(180, 180, 180);
    m_colorRedLine = QColor(100, 40, 40);
    m_colorGreenLine = QColor(40, 100, 40);
    m_colorBlueLine = QColor(40, 40, 100);
    m_colorAlphaLine = QColor(100,100,100);
  }
}

void Q1DTransferFunction::changeEvent(QEvent * event) {
  // call superclass method
  QWidget::changeEvent(event);

  if (event->type() == QEvent::EnabledChange) {
    SetColor(isEnabled());
    m_bBackdropCacheUptodate = false;
    update();
  }
}


void Q1DTransferFunction::RedrawPreviewBarBack() {
  int w = MathTools::MakeMultiple<int>(width()-(m_iLeftBorder+3+m_iRightBorder), 8);
  int h = MathTools::MakeMultiple<int>(m_iTopPreviewHeight, 8);

  delete m_pPreviewBack;
  m_pPreviewBack = new QImage(w,h,QImage::Format_RGB32);
  unsigned int iCheckerboard[2] = {qRgb(128,128,128), qRgb(0,0,0)};
  int iCurrent = 0;
  for (int y = 0;y<h;y++) {
    int iLastCurrent = iCurrent;
    for (int x = 0;x<w;x++) {
      if (x%8 == 0) iCurrent = 1-iCurrent;
      m_pPreviewBack->setPixel(x,y,iCheckerboard[iCurrent]);
    }
    iCurrent = (y != 0 && y%8 == 0) ? 1-iLastCurrent : iLastCurrent;
  }
}

void Q1DTransferFunction::paintEvent(QPaintEvent *event) {
  // call superclass method
  QWidget::paintEvent(event);

  if (m_trans.isValid(m_MasterController.LuaScript()) == false) {
    QPainter painter(this);
    DrawCoordinateSystem(painter);
    return;
  }

  // as drawing the histogram can become quite expensive we'll cache it in an image and only redraw if needed
  if (!m_bBackdropCacheUptodate || (unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {

    RedrawPreviewBarBack();

    // delete the old pixmap an create a new one if the size has changed
    if ((unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {
      delete m_pBackdropCache;
      m_pBackdropCache = new QPixmap(width(),height());
    }

    // attach a painter to the pixmap
    QPainter image_painter(m_pBackdropCache);

    // draw the backdrop into the image
    DrawCoordinateSystem(image_painter);
    DrawHistogram(image_painter);

    // update change detection states
    m_bBackdropCacheUptodate = true;
    m_iCachedHeight = height();
    m_iCachedWidth = width();
  }

  // now draw everything rest into this widget
  QPainter painter(this);

  // the image captured before (or cached from a previous call)
  painter.drawImage(0,0,m_pBackdropCache->toImage());

  // and the function plots
  DrawFunctionPlots(painter);
}

bool Q1DTransferFunction::QLoadFromFile(const QString& strFilename) {
  return this->LoadFromFile(strFilename.toStdWString());
}

bool Q1DTransferFunction::LoadFromFile(const std::wstring& strFilename) {
  // hand the load call over to the TransferFunction1D class
  size_t iSize = 0;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(m_MasterController.LuaScript())) {
    iSize = ss->cexecRet<size_t>(m_trans.fqName() + ".getSize");
  }

  if (ss->cexecRet<bool>(m_trans.fqName() + ".loadFromFileWithSize", 
                         strFilename, iSize)) {
    PreparePreviewData();
    update();
    ApplyFunction();
    return true;
  } else return false;
}

bool Q1DTransferFunction::AddFromFile(const QString& strFilename) {
  size_t iSize = 0;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(ss)) {
    iSize = ss->cexecRet<size_t>(m_trans.fqName() + ".getSize");
  } else return false;

  // Retrieve color data from Lua script class.
  shared_ptr<vector<FLOATVECTOR4> > cdata = 
      ss->cexecRet<shared_ptr<vector<FLOATVECTOR4> > >(
          m_trans.fqName() + ".getColorData");

  TransferFunction1D other(iSize);
  if( other.Load(strFilename.toStdWString(), iSize) ) {

    for (size_t i = 0;i<iSize;i++)
      (*cdata)[i] += other.GetColor(i);

    PreparePreviewData();
    update();
    ApplyFunction();
    return true;
  } else {
    return false;
  }
}

bool Q1DTransferFunction::SubtractFromFile(const QString& strFilename) {
  size_t iSize = 0;
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (m_trans.isValid(ss)) {
    iSize = ss->cexecRet<size_t>(m_trans.fqName() + ".getSize");
  } else return false;

  // Retrieve color data from Lua script class.
  shared_ptr<vector<FLOATVECTOR4> > cdata = 
      ss->cexecRet<shared_ptr<vector<FLOATVECTOR4> > >(
          m_trans.fqName() + ".getColorData");

  TransferFunction1D other(iSize);
  if( other.Load(strFilename.toStdWString(), iSize) ) {

    for (size_t i = 0;i<iSize;i++)
      (*cdata)[i] -= other.GetColor(i);

    PreparePreviewData();
    update();
    ApplyFunction();
    return true;
  } else {
    return false;
  }
}

bool Q1DTransferFunction::SaveToFile(const QString& strFilename) {
  // hand the save call over to the TransferFunction1D class
  std::shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  return ss->cexecRet<bool>(m_trans.fqName() + ".save", 
                            strFilename.toStdWString());
}
