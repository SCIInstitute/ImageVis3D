#include "Q2DTransferFunction.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

using namespace std;

Q2DTransferFunction::Q2DTransferFunction(QWidget *parent) :
	QWidget(parent),
	m_pTrans(NULL),
	m_iPaintmode(Q2DT_PAINT_NONE),
	m_iActiveSwatchIndex(-1),
	m_bBackdropCacheUptodate(false),
	m_iCachedHeight(0),
	m_iCachedWidth(0),
	m_pBackdropCache(NULL),
	
	// border size, may be changed arbitrarily
	m_iBorderSize(4),
	m_iSwatchBorderSize(5),

	// mouse motion
	m_iLastIndex(-1),
	m_fLastValue(0)
{
	SetColor(isEnabled());
}

Q2DTransferFunction::~Q2DTransferFunction(void)
{
	// delete the backdrop cache pixmap
	delete m_pBackdropCache;
}

QSize Q2DTransferFunction::minimumSizeHint() const
{
	return QSize(50, 50);
}

QSize Q2DTransferFunction::sizeHint() const
{
	return QSize(400, 400);
}

void Q2DTransferFunction::SetData(const Histogram2D* vHistrogram, TransferFunction2D* pTrans) {
	if (pTrans == NULL) return;

	// resize the histogram vector
	m_vHistrogram.Resize(vHistrogram->GetSize());
	
	// also resize the transferfunction
	m_pTrans = pTrans;

	// force the draw routine to recompute the backdrop cache
	m_bBackdropCacheUptodate = false;

	// if the histogram is empty we are done
	if (m_vHistrogram.GetSize().area() == 0)  return;

	// rescale the histogram to the [0..1] range
	// first find min and max ...
	unsigned int iMax = vHistrogram->GetLinear(0);
	unsigned int iMin = iMax;
	for (size_t i = 0;i<m_vHistrogram.GetSize().area();i++) {
		unsigned int iVal = vHistrogram->GetLinear(i);
		if (iVal > iMax) iMax = iVal;
		if (iVal < iMin) iMin = iVal;
	}

	// ... than rescale
	float fDiff = float(iMax)-float(iMin);
	for (size_t i = 0;i<m_vHistrogram.GetSize().area();i++)
		m_vHistrogram.SetLinear(i, (float(vHistrogram->GetLinear(i)) - float(iMin)) / fDiff);


	// debug
	{
		TFPolygon mySwatch;
		mySwatch.pPoints.push_back(FLOATVECTOR2(1.0f,0.0f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.8f,0.0f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.8f,0.8f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(1.0f,1.0f));
	
		mySwatch.pGradientCoords[0] = FLOATVECTOR2(0.1f,0.1f);
		mySwatch.pGradientCoords[1] = FLOATVECTOR2(0.5f,0.5f);

		mySwatch.pGradientStops.push_back(GradientStop(0.0f,FLOATVECTOR4(0,0,0,0)));
		mySwatch.pGradientStops.push_back(GradientStop(0.5f,FLOATVECTOR4(1,0,0,1)));
		mySwatch.pGradientStops.push_back(GradientStop(1.0f,FLOATVECTOR4(0,0,0,0)));
		
		m_pTrans->m_Swatches.push_back(mySwatch);
	}
	{
		TFPolygon mySwatch;
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.0f,0.5f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.2f,0.5f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.2f,0.8f));
		mySwatch.pPoints.push_back(FLOATVECTOR2(0.0f,0.8f));
		
		mySwatch.pGradientCoords[0] = FLOATVECTOR2(0.0f,0.7f);
		mySwatch.pGradientCoords[1] = FLOATVECTOR2(0.2f,0.7f);

		mySwatch.pGradientStops.push_back(GradientStop(0.1f,FLOATVECTOR4(0,0,1,1)));
		mySwatch.pGradientStops.push_back(GradientStop(0.5f,FLOATVECTOR4(1,1,1,0.0f)));
		mySwatch.pGradientStops.push_back(GradientStop(1.0f,FLOATVECTOR4(0,0,1,1)));

		m_pTrans->m_Swatches.push_back(mySwatch);
	}


	m_iActiveSwatchIndex = 0;


}

void Q2DTransferFunction::DrawBorder(QPainter& painter) {
	// draw background with border
	QPen borderPen(m_colorBorder, m_iBorderSize, Qt::SolidLine);
	painter.setPen(borderPen);

	painter.setBrush(m_colorBack);
	QRect backRect(0,0,width(),height());
	painter.drawRect(backRect);
}

void Q2DTransferFunction::DrawHistogram(QPainter& painter, float fScale) {
	if (m_pTrans == NULL) return;

	// convert the histogram into an image
	// define the bitmap ...
	QImage image(QSize(m_vHistrogram.GetSize().x, m_vHistrogram.GetSize().y), QImage::Format_RGB32);
	for (size_t y = 0;y<m_vHistrogram.GetSize().y;y++) 
		for (size_t x = 0;x<m_vHistrogram.GetSize().x;x++) {
			float value = min(1.0f, fScale*m_vHistrogram.Get(x,y));
			image.setPixel(x,y,qRgb(int(m_colorBack.red()*(1.0f-value)   + m_colorHistogram.red()*value),
				                    int(m_colorBack.green()*(1.0f-value) + m_colorHistogram.green()*value),
									int(m_colorBack.blue()*(1.0f-value)  + m_colorHistogram.blue()*value)));
		}

	// ... draw it
    QRectF target(m_iBorderSize/2, m_iBorderSize/2, width()-m_iBorderSize, height()-m_iBorderSize);
    QRectF source(0.0, 0.0, m_vHistrogram.GetSize().x, m_vHistrogram.GetSize().y);
	painter.drawImage( target, image, source );
}



INTVECTOR2 Q2DTransferFunction::Rel2Abs(FLOATVECTOR2 vfCoord) {
	return INTVECTOR2(int(m_iSwatchBorderSize/2+m_iBorderSize/2+vfCoord.x*(width()-m_iBorderSize-m_iSwatchBorderSize)),
					  int(m_iSwatchBorderSize/2+m_iBorderSize/2+vfCoord.y*(height()-m_iBorderSize-m_iSwatchBorderSize)));
}

void Q2DTransferFunction::DrawSwatches(QPainter& painter) {
	if (m_pTrans == NULL) return;

	painter.setRenderHint(painter.Antialiasing, true);
	painter.translate(+0.5, +0.5);  // TODO: check if we need this

	QPen borderPen(m_colorSwatchBorder,       m_iSwatchBorderSize, Qt::SolidLine);
	QPen circlePen(m_colorSwatchBorderCircle, m_iSwatchBorderSize, Qt::SolidLine);
	QPen gradCircePen(m_colorSwatchGradCircle, m_iSwatchBorderSize/2, Qt::SolidLine);


	QBrush solidBrush = QBrush(m_colorSwatchBorderCircle, Qt::SolidPattern);

	// render swatches
	for (size_t i = 0;i<m_pTrans->m_Swatches.size();i++) {
		TFPolygon& currentSwatch = m_pTrans->m_Swatches[i];
		
		std::vector<QPoint> pointList(currentSwatch.pPoints.size());
		for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {		
			INTVECTOR2 vPixelPos = Rel2Abs(currentSwatch.pPoints[j]);
			pointList[j] = QPoint(vPixelPos.x, vPixelPos.y);
		}

		INTVECTOR2 vPixelPos0 = Rel2Abs(currentSwatch.pGradientCoords[0])-m_iSwatchBorderSize, vPixelPos1 = Rel2Abs(currentSwatch.pGradientCoords[1])-m_iSwatchBorderSize; 
		QLinearGradient linearBrush(vPixelPos0.x, vPixelPos0.y, vPixelPos1.x, vPixelPos1.y);
		
		for (size_t j = 0;j<currentSwatch.pGradientStops.size();j++) {			
			linearBrush.setColorAt(currentSwatch.pGradientStops[j].first, 
								   QColor(int(currentSwatch.pGradientStops[j].second[0]*255),
										  int(currentSwatch.pGradientStops[j].second[1]*255),
								          int(currentSwatch.pGradientStops[j].second[2]*255),
								          int(currentSwatch.pGradientStops[j].second[3]*255)));
		}

		painter.setPen(borderPen);
		painter.setBrush(linearBrush);
		painter.drawPolygon(&pointList[0], currentSwatch.pPoints.size());
		painter.setBrush(Qt::NoBrush);

		if (m_iActiveSwatchIndex == int(i)) {
			painter.setPen(circlePen);
			painter.setBrush(solidBrush);
			for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {		
				painter.drawEllipse(pointList[j].x()-m_iSwatchBorderSize, pointList[j].y()-m_iSwatchBorderSize, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
			}

			painter.setBrush(Qt::NoBrush);
			painter.setPen(gradCircePen);
			INTVECTOR2 vPixelPos = Rel2Abs(currentSwatch.pGradientCoords[0])-m_iSwatchBorderSize;
			painter.drawEllipse(vPixelPos.x, vPixelPos.y, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
			vPixelPos = Rel2Abs(currentSwatch.pGradientCoords[1])-m_iSwatchBorderSize;
			painter.drawEllipse(vPixelPos.x, vPixelPos.y, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);			
		}
	}
	painter.setRenderHint(painter.Antialiasing, false);
}

void Q2DTransferFunction::mousePressEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mousePressEvent(event);
	// clear the "last position" index
	m_iLastIndex = -1;
}

void Q2DTransferFunction::mouseReleaseEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mouseReleaseEvent(event);
	// clear the "last position" index
	m_iLastIndex = -1;
}

void Q2DTransferFunction::mouseMoveEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mouseMoveEvent(event);

/*	// exit if nothing is to be painted
	if (m_iPaintmode == Q2DT_PAINT_NONE) return;

	// compute some grid dimensions
	unsigned int iGridWidth  = width()-(m_iLeftBorder+m_iRightBorder)-3;
	unsigned int iGridHeight = height()-(m_iBottomBorder+m_iTopBorder)-2;
	unsigned int iVectorSize = m_pTrans->pColorData.size();

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
			m_pTrans->pColorData[iIndex][0] = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_GREEN) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_pTrans->pColorData[iIndex][1] = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_BLUE) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_pTrans->pColorData[iIndex][2] = _fValueMin;
			_fValueMin += fValueInc;
		}
	}
	if (m_iPaintmode & Q2DT_PAINT_ALPHA) {
		float _fValueMin = fValueMin;
		for (int iIndex = iIndexMin;iIndex<=iIndexMax;++iIndex) {
			m_pTrans->pColorData[iIndex][3] = _fValueMin;
			_fValueMin += fValueInc;
		}
	}

	// redraw this widget
	update();

	*/
}

void Q2DTransferFunction::SetColor(bool bIsEnabled) {
		if (bIsEnabled) {
			m_colorHistogram = QColor(255,255,255);
			m_colorBack = QColor(Qt::black);
			m_colorBorder = QColor(255, 255, 255);
			m_colorSwatchBorder = QColor(255, 0, 0);
			m_colorSwatchBorderCircle = QColor(255, 255, 0);
			m_colorSwatchGradCircle = QColor(0, 255, 0);
		} else {
			m_colorHistogram = QColor(55,55,55);
			m_colorBack = QColor(Qt::black);
			m_colorBorder = QColor(100, 100, 100);
			m_colorSwatchBorder = QColor(100, 50, 50);
			m_colorSwatchBorderCircle = QColor(100, 100, 50);
			m_colorSwatchGradCircle = QColor(50, 100, 50);
		}
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

		// draw the backdrop into the image
		DrawBorder(image_painter);
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

	// and the swatches
	DrawSwatches(painter);
}

bool Q2DTransferFunction::LoadFromFile(const QString& strFilename) {
	// hand the load call over to the TransferFunction1D class
	if( m_pTrans->Load(strFilename.toStdString()) ) {
		update();
		return true;
	} else return false;
}

bool Q2DTransferFunction::SaveToFile(const QString& strFilename) {
	// hand the save call over to the TransferFunction1D class
	return m_pTrans->Save(strFilename.toStdString());
}
