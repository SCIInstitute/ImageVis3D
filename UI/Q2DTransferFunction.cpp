#include "Q2DTransferFunction.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

Q2DTransferFunction::Q2DTransferFunction(QWidget *parent) :
	QWidget(parent),
	m_iPaintmode(Q2DT_PAINT_NONE),
	m_bBackdropCacheUptodate(false),
	m_iCachedHeight(0),
	m_iCachedWidth(0),
	m_pBackdropCache(NULL),
	// borders, may be changed arbitrarly
	m_iLeftBorder(20),
	m_iBottomBorder(20),
	// automatically computed borders
	m_iRightBorder(0),
	m_iTopBorder(0),
	// colors, may be changed arbitrarily 
	m_colorHistogram(50,50,50),
	m_colorBack(Qt::black),
	m_colorBorder(100, 100, 255),
	m_colorScale(100, 100, 255),
	m_colorLargeScale(180, 180, 180),
	m_colorRedLine(255, 0, 0),
	m_colorGreenLine(0, 255, 0),
	m_colorBlueLine(0, 0, 255),
	m_colorAlphaLine(Qt::white),
	// scale apearance, may be changed arbitrarily (except for the marker length wich should both be less or equal to m_iLeftBorder and m_iBottomBorder)
	m_iMarkersX(40),
	m_iMarkersY (40),
	m_iBigMarkerSpacingX(5),
	m_iBigMarkerSpacingY(5),
	m_iMarkerLength(5),
	m_iBigMarkerLength(m_iMarkerLength*3),
	// mouse motion
	m_iLastIndex(-1),
	m_fLastValue(0)
{
}

Q2DTransferFunction::~Q2DTransferFunction(void)
{
	// delete the backdrop cache pixmap
	delete m_pBackdropCache;
}

void Q2DTransferFunction::SetHistogram(std::vector<unsigned int> vHistrogram) {
	// resize the histogram vector
	m_vHistrogram.resize(vHistrogram.size());
	
	// also resize the transferfunction
	m_Trans.Resize(vHistrogram.size());

	// force the draw routine to recompute the backdrop cache
	m_bBackdropCacheUptodate = false;

	// if the histogram is empty we are done
	if (m_vHistrogram.size() == 0)  return;

	// rescale the histogram to the [0..1] range
	// first find min and max ...
	float fMax = float(vHistrogram[0]), fMin = float(vHistrogram[0]);
	for (size_t i = 0;i<m_vHistrogram.size();i++) {
		if (float(vHistrogram[i]) > fMax) fMax = float(vHistrogram[i]);
		if (float(vHistrogram[i]) < fMin) fMin = float(vHistrogram[i]);

		for (unsigned int j = 0;j<4;j++) m_Trans.pColorData[i][j] = 0.0f;
	}

	// ... than rescale
	float fDiff = fMax-fMin;
	for (size_t i = 0;i<m_vHistrogram.size();i++) {
		m_vHistrogram[i]  = vHistrogram[i] - fMin;
		m_vHistrogram[i] /= fDiff;
	}
}

void Q2DTransferFunction::DrawCoordinateSystem(QPainter& painter) {
	// adjust left and bottom border so that the marker count can be met
	m_iRightBorder   = (width()-(m_iLeftBorder+2))   %m_iMarkersX;
	m_iTopBorder     = (height()-(m_iBottomBorder+2))%m_iMarkersY;

	// compute the actual marker spaceing
	unsigned int iMarkerSpacingX = (width()-(m_iLeftBorder+2))/m_iMarkersX;
	unsigned int iMarkerSpacingY = (height()-(m_iBottomBorder+2))/m_iMarkersY;

	// draw background
	painter.setBrush(m_colorBack);
	QRect backRect(0,0,width(),height());
	painter.drawRect(backRect);

	// draw grid borders
	QPen borderPen(m_colorBorder, 1, Qt::SolidLine);
	painter.setPen(borderPen);
	QRect borderRect(m_iLeftBorder,m_iTopBorder, width()-(m_iLeftBorder+2+m_iRightBorder), height()-(m_iTopBorder+m_iBottomBorder+2));
	painter.drawRect(borderRect);

	// draw Y axis markers
	QPen penScale(m_colorScale, 1, Qt::SolidLine);
	QPen penLargeScale(m_colorLargeScale, 1, Qt::SolidLine);
	for (unsigned int i = 0;i<m_iMarkersY;i++) {
		int iPosY = height()-m_iBottomBorder-2 - i*iMarkerSpacingY; 

		if (i%m_iBigMarkerSpacingY == 0) {
			painter.setPen(penLargeScale);
			painter.drawLine(m_iLeftBorder-m_iBigMarkerLength, iPosY, m_iLeftBorder, iPosY);
		} else {
			painter.setPen(penScale);
			painter.drawLine(m_iLeftBorder-m_iMarkerLength, iPosY, m_iLeftBorder, iPosY);
		}
	}

	// draw X axis markers
	unsigned int iStartY = height()-(m_iBottomBorder+2);
	for (unsigned int i = 0;i<m_iMarkersX;i++) {
		int iPosX = m_iLeftBorder+i*iMarkerSpacingX;
		if (i%m_iBigMarkerSpacingX == 0) {
			painter.setPen(penLargeScale);
			painter.drawLine(iPosX, iStartY+m_iBigMarkerLength, iPosX, iStartY);
		} else {
			painter.setPen(penScale);
			painter.drawLine(iPosX, iStartY+m_iMarkerLength, iPosX, iStartY);
		}
	}
}

void Q2DTransferFunction::DrawHistogram(QPainter& painter) {
	// nothing todo if the histogram is empty
	if (m_vHistrogram.size() == 0) return;

	// compute some grid dimensions
	unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
	unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;

	// draw the histogram a as large polygon
	// define the polygon ...
	std::vector<QPointF> pointList;
	pointList.push_back(QPointF(m_iLeftBorder+1, iGridHeight-m_iBottomBorder));
	for (size_t i = 0;i<m_vHistrogram.size();i++) 
		pointList.push_back(QPointF(m_iLeftBorder+1+float(iGridWidth)*i/(m_vHistrogram.size()-1), m_iTopBorder+iGridHeight-m_vHistrogram[i]*iGridHeight));	
	pointList.push_back(QPointF(m_iLeftBorder+iGridWidth, m_iTopBorder+iGridHeight));
	pointList.push_back(QPointF(m_iLeftBorder+1, m_iTopBorder+iGridHeight));

	// ... draw it
	painter.setPen(Qt::NoPen);
	painter.setBrush(m_colorHistogram);
	painter.drawPolygon(&pointList[0], pointList.size());
}

void Q2DTransferFunction::DrawFunctionPlots(QPainter& painter) {
	// nothing todo if the histogram is empty
	if (m_vHistrogram.size() == 0) return;

	// compute some grid dimensions
	unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
	unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;

	// draw the tranfer function as one larger polyline
	std::vector<QPointF> pointList(m_Trans.pColorData.size());
	QPen penCurve(m_colorBorder, 1, Qt::SolidLine);
	
	// for every component
	for (unsigned int j = 0;j<4;j++) {

		// select the color
		switch (j) {
			case 0  : penCurve.setColor(m_colorRedLine);   break;
			case 1  : penCurve.setColor(m_colorGreenLine); break;
			case 2  : penCurve.setColor(m_colorBlueLine);  break;
			default : penCurve.setColor(m_colorAlphaLine); break;
		}

		// define the polyline
		for (size_t i = 0;i<pointList.size();i++) {
			pointList[i]= QPointF(m_iLeftBorder+1+float(iGridWidth)*i/(pointList.size()-1),
						          m_iTopBorder+iGridHeight-m_Trans.pColorData[i][j]*iGridHeight);
		}

		// draw the polyline
		painter.setPen(penCurve);
		painter.drawPolyline(&pointList[0], pointList.size());
	}

}

void Q2DTransferFunction::mousePressEvent(QMouseEvent *event) {
	// call superclass method
	QWidget::mousePressEvent(event);
	// clear the "last position" index
	m_iLastIndex = -1;
}

void Q2DTransferFunction::mouseReleaseEvent(QMouseEvent *event) {
	// call superclass method
	QWidget::mouseReleaseEvent(event);
	// clear the "last position" index
	m_iLastIndex = -1;
}

void Q2DTransferFunction::mouseMoveEvent(QMouseEvent *event) {
	// call superclass method
	QWidget::mouseMoveEvent(event);

	// exit if nothing is to be painted
	if (m_iPaintmode == Q2DT_PAINT_NONE) return;

	// compute some grid dimensions
	unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
	unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;
	unsigned int iVectorSize = m_Trans.pColorData.size();

	// compute position in color array
	int iCurrentIndex = int((float(event->x())-float(m_iLeftBorder)-1.0f)*float(iVectorSize-1)/float(iGridWidth));
	iCurrentIndex = std::min<int>(iVectorSize-1, std::max<int>(0,iCurrentIndex));

	// compute actual color value 
	float fValue = (float(m_iTopBorder)+float(iGridHeight)-float(event->y()))/float(iGridHeight);
	fValue	  = std::min<float>(1.0f, std::max<float>(0.0f,fValue));

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
	if (m_iPaintmode & Q2DT_PAINT_RED) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_Trans.pColorData[iIndex].r = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_GREEN) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_Trans.pColorData[iIndex].g = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_BLUE) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_Trans.pColorData[iIndex].b = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_ALPHA) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_Trans.pColorData[iIndex].a = _fValueMin;
			_fValueMin += fValueInc;
		}
	}

	// redraw this widget
	update();
}

void Q2DTransferFunction::paintEvent(QPaintEvent *event) {
	// call superclass method
	QWidget::paintEvent(event);

	// as drawing the histogram can become quite expensive we'll cache it in an image and only redraw if needed
	if (!m_bBackdropCacheUptodate || (unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {
		
		// delete the old pixmap an create a new one if the size has changed
		if ((unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {
			delete m_pBackdropCache;
			m_pBackdropCache = new QPixmap(width(),height());
		}

		// attach a painter to the pixmap
		QPainter image_painter(m_pBackdropCache);

		// draw the abckdrop into the image
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

	// and the funtion plots
	DrawFunctionPlots(painter);
}

bool Q2DTransferFunction::LoadFromFile(const QString& strFilename) {
	// hand the load call over to the TransferFunction1D class
	if( m_Trans.Load(strFilename.toStdString()) ) {
		update();
		return true;
	} else return false;
}

bool Q2DTransferFunction::SaveToFile(const QString& strFilename) {
	// hand the save call over to the TransferFunction1D class
	return m_Trans.Save(strFilename.toStdString());
}
