#include "Q2DTransferFunction.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include "../Controller/MasterController.h"
#include <limits>

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

using namespace std;

Q2DTransferFunction::Q2DTransferFunction(MasterController& masterController, QWidget *parent) :
	QWidget(parent),
	m_pTrans(NULL),
	m_iPaintmode(Q2DT_PAINT_NONE),
	m_iActiveSwatchIndex(-1),
	m_MasterController(masterController),
	m_bBackdropCacheUptodate(false),
	m_iCachedHeight(0),
	m_iCachedWidth(0),
	m_pBackdropCache(NULL),
	
	// border size, may be changed arbitrarily
	m_iBorderSize(4),
	m_iSwatchBorderSize(5),

	// mouse motion
	m_iPointSelIndex(-1),
	m_iGradSelIndex(-1),
	m_vMousePressPos(0,0),
	m_bDragging(false)
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
	m_pTrans = pTrans;
	if (m_pTrans == NULL) return;

	// resize the histogram vector
	m_vHistrogram.Resize(vHistrogram->GetSize());
	
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

FLOATVECTOR2 Q2DTransferFunction::Abs2Rel(INTVECTOR2 vCoord) {
	return FLOATVECTOR2((float(vCoord.x)-m_iSwatchBorderSize/2.0f+m_iBorderSize/2.0f)/float(width()-m_iBorderSize-m_iSwatchBorderSize),
					    (float(vCoord.y)-m_iSwatchBorderSize/2.0f+m_iBorderSize/2.0f)/float(height()-m_iBorderSize-m_iSwatchBorderSize));
}

void Q2DTransferFunction::DrawSwatches(QPainter& painter) {
	if (m_pTrans == NULL) return;

	static bool t = true;
	if (!t) return;

	painter.setRenderHint(painter.Antialiasing, true);
	painter.translate(+0.5, +0.5);  // TODO: check if we need this

	QPen borderPen(m_colorSwatchBorder,       m_iSwatchBorderSize, Qt::SolidLine);
	QPen circlePen(m_colorSwatchBorderCircle, m_iSwatchBorderSize, Qt::SolidLine);
	QPen gradCircePen(m_colorSwatchGradCircle, m_iSwatchBorderSize/2, Qt::SolidLine);
	QPen circlePenSel(m_colorSwatchBorderCircleSel, m_iSwatchBorderSize, Qt::SolidLine);
	QPen gradCircePenSel(m_colorSwatchGradCircleSel, m_iSwatchBorderSize/2, Qt::SolidLine);

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
			painter.setBrush(solidBrush);
			for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {		
				if (m_iPointSelIndex == int(j)) painter.setPen(circlePenSel); else painter.setPen(circlePen);
				painter.drawEllipse(pointList[j].x()-m_iSwatchBorderSize, pointList[j].y()-m_iSwatchBorderSize, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
			}

			painter.setBrush(Qt::NoBrush);
			if (m_iGradSelIndex== 0) painter.setPen(gradCircePenSel); else painter.setPen(gradCircePen);
			INTVECTOR2 vPixelPos = Rel2Abs(currentSwatch.pGradientCoords[0])-m_iSwatchBorderSize;
			painter.drawEllipse(vPixelPos.x, vPixelPos.y, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);
			vPixelPos = Rel2Abs(currentSwatch.pGradientCoords[1])-m_iSwatchBorderSize;
			if (m_iGradSelIndex== 1) painter.setPen(gradCircePenSel); else painter.setPen(gradCircePen);
			painter.drawEllipse(vPixelPos.x, vPixelPos.y, m_iSwatchBorderSize*2, m_iSwatchBorderSize*2);			
		}
	}
	painter.setRenderHint(painter.Antialiasing, false);
}

void Q2DTransferFunction::mousePressEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mousePressEvent(event);

	if (m_iActiveSwatchIndex >= 0 && m_iActiveSwatchIndex<int(m_pTrans->m_Swatches.size())) {
		m_vMousePressPos = INTVECTOR2(event->x(), event->y());
		TFPolygon& currentSwatch = m_pTrans->m_Swatches[m_iActiveSwatchIndex];

		// left mouse drags points around
		if (event->button() == Qt::LeftButton) {

			m_bDragging = true;

			m_iPointSelIndex = -1;
			m_iGradSelIndex = -1;

			// find closest corner point
			float fMinDist = std::numeric_limits<float>::max();
			for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
				INTVECTOR2 point = Rel2Abs(currentSwatch.pPoints[j]);

				float fDist = sqrt( float(m_vMousePressPos.x-point.x)*float(m_vMousePressPos.x-point.x) +  float(m_vMousePressPos.y-point.y)*float(m_vMousePressPos.y-point.y) );

				if (fMinDist > fDist) {
					fMinDist = fDist;
					m_iPointSelIndex = j;
					m_iGradSelIndex = -1;
				}
			}

			// find closest gradient coord
			for (size_t j = 0;j<2;j++) {
				INTVECTOR2 point = Rel2Abs(currentSwatch.pGradientCoords[j]);

				float fDist = sqrt( float(m_vMousePressPos.x-point.x)*float(m_vMousePressPos.x-point.x) +  float(m_vMousePressPos.y-point.y)*float(m_vMousePressPos.y-point.y) );

				if (fMinDist > fDist) {
					fMinDist = fDist;
					m_iPointSelIndex = -1;
					m_iGradSelIndex = j;
				}
			}

		}

		// right mouse removes / adds points
		if (event->button() == Qt::RightButton) {

			FLOATVECTOR2 vfP = Abs2Rel(m_vMousePressPos);

			// find closest edge and compute the point on that edge
			float fMinDist = std::numeric_limits<float>::max();
			FLOATVECTOR2 vfInserCoord;
			int iInsertIndex = -1;

			for (size_t j = 0;j<currentSwatch.pPoints.size();j++) {
				FLOATVECTOR2 A = currentSwatch.pPoints[j];
				FLOATVECTOR2 B = currentSwatch.pPoints[(j+1)%currentSwatch.pPoints.size()];

				// check if we are deleting a point				
				if (currentSwatch.pPoints.size() > 3) {
					INTVECTOR2 vPixelDist = Rel2Abs(vfP-A);
					if ( sqrt( float(vPixelDist.x*vPixelDist.x+vPixelDist.y*vPixelDist.y)) <= m_iSwatchBorderSize*3) {
						currentSwatch.pPoints.erase(currentSwatch.pPoints.begin()+j);
						iInsertIndex = -1;
						emit SwatchChange();
						break;
					}
				}


				FLOATVECTOR2 C = vfP - A;		// Vector from a to Point
				float d = (B - A).length();		// Length of the line segment
				FLOATVECTOR2 V = (B - A)/d;		// Unit Vector from A to B
				float t = V^C;					// Intersection point Distance from A

				float fDist;
				if (t >= 0 && t <= d) 
					fDist = (vfP-(A + V*t)).length(); 
				else 
					fDist = std::numeric_limits<float>::max();


				if (fDist < fMinDist) {
					fMinDist = fDist;
					vfInserCoord = vfP;
					iInsertIndex = j+1;
				}

			}

			if (iInsertIndex >= 0) {
				currentSwatch.pPoints.insert(currentSwatch.pPoints.begin()+iInsertIndex, vfInserCoord);
				emit SwatchChange();
			}
		}
		update();
	}
}

void Q2DTransferFunction::mouseReleaseEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mouseReleaseEvent(event);

	m_bDragging = false;
	m_iPointSelIndex = -1;
	m_iGradSelIndex = -1;

	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	update();
}

void Q2DTransferFunction::mouseMoveEvent(QMouseEvent *event) {
	if (m_pTrans == NULL) return;
	// call superclass method
	QWidget::mouseMoveEvent(event);

	if (m_bDragging) {

		INTVECTOR2 vMouseCurrentPos(event->x(), event->y());

		FLOATVECTOR2 vfPressPos = Abs2Rel(m_vMousePressPos);
		FLOATVECTOR2 vfCurrentPos = Abs2Rel(vMouseCurrentPos);

		FLOATVECTOR2 vfDelta = vfCurrentPos-vfPressPos;
		
		TFPolygon& currentSwatch = m_pTrans->m_Swatches[m_iActiveSwatchIndex];
		if (m_iPointSelIndex >= 0)  {
			currentSwatch.pPoints[m_iPointSelIndex] += vfDelta;
		} else {
			currentSwatch.pGradientCoords[m_iGradSelIndex] += vfDelta;
		}

		m_vMousePressPos = vMouseCurrentPos;

		update();
	}
}

void Q2DTransferFunction::SetColor(bool bIsEnabled) {
		if (bIsEnabled) {
			m_colorHistogram = QColor(255,255,255);
			m_colorBack = QColor(Qt::black);
			m_colorBorder = QColor(255, 255, 255);
			m_colorSwatchBorder = QColor(255, 0, 0);
			m_colorSwatchBorderCircle = QColor(255, 255, 0);
			m_colorSwatchGradCircle = QColor(0, 255, 0);
			m_colorSwatchGradCircleSel = QColor(255, 255, 255);
			m_colorSwatchBorderCircleSel = QColor(255, 255, 255);
		} else {
			m_colorHistogram = QColor(55,55,55);
			m_colorBack = QColor(Qt::black);
			m_colorBorder = QColor(100, 100, 100);
			m_colorSwatchBorder = QColor(100, 50, 50);
			m_colorSwatchBorderCircle = QColor(100, 100, 50);
			m_colorSwatchGradCircle = QColor(50, 100, 50);
			m_colorSwatchGradCircleSel = m_colorSwatchGradCircle;
			m_colorSwatchBorderCircleSel = m_colorSwatchBorderCircle;
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


void Q2DTransferFunction::Draw1DTrans(QPainter& painter) {
	QImage image1DTrans(m_pTrans->m_Trans1D.vColorData.size(),1, QImage::Format_ARGB32);

	for (unsigned int x = 0;x<m_pTrans->m_Trans1D.vColorData.size();x++) {
		image1DTrans.setPixel(x,0,qRgba(m_pTrans->m_Trans1D.vColorData[x][0]*255,
									   m_pTrans->m_Trans1D.vColorData[x][1]*255,
									   m_pTrans->m_Trans1D.vColorData[x][2]*255,
									   m_pTrans->m_Trans1D.vColorData[x][3]*255));
	}

	QRect imageRect(m_iBorderSize/2, m_iBorderSize/2, width()-m_iBorderSize, height()-m_iBorderSize);
	painter.drawImage(imageRect,image1DTrans);
}

void Q2DTransferFunction::paintEvent(QPaintEvent *event) {
	// call superclass method
	QWidget::paintEvent(event);
	
	if (m_pTrans == NULL) {
		QPainter painter(this);
		DrawBorder(painter);
		return;
	}

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

		Draw1DTrans(image_painter);

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
		m_iActiveSwatchIndex = 0;
		m_bBackdropCacheUptodate = false;
		update();
		m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
		emit SwatchChange();
		return true;
	} else return false;
}

bool Q2DTransferFunction::SaveToFile(const QString& strFilename) {
	// hand the save call over to the TransferFunction1D class
	return m_pTrans->Save(strFilename.toStdString());
}


void Q2DTransferFunction::Set1DTrans(const TransferFunction1D* p1DTrans) {
	m_pTrans->m_Trans1D = TransferFunction1D(*p1DTrans);
	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	m_iActiveSwatchIndex = 0;
	m_bBackdropCacheUptodate = false;
	update();
}

void Q2DTransferFunction::SetActiveSwatch(const int iActiveSwatch) {
	if (iActiveSwatch == -1 && m_pTrans->m_Swatches.size() > 0) return;
	m_iActiveSwatchIndex = iActiveSwatch;
	update();
}

void Q2DTransferFunction::AddSwatch() {	
	TFPolygon newSwatch;

	newSwatch.pPoints.push_back(FLOATVECTOR2(0,0));
	newSwatch.pPoints.push_back(FLOATVECTOR2(0,1));
	newSwatch.pPoints.push_back(FLOATVECTOR2(1,1));
	newSwatch.pPoints.push_back(FLOATVECTOR2(1,0));

	newSwatch.pGradientCoords[0] = FLOATVECTOR2(0,0.5);
	newSwatch.pGradientCoords[1] = FLOATVECTOR2(1,0.5);

	GradientStop g1(0,FLOATVECTOR4(0,0,0,0)),g2(0.5f,FLOATVECTOR4(1,1,1,1)),g3(1,FLOATVECTOR4(0,0,0,0));
	newSwatch.pGradientStops.push_back(g1);
	newSwatch.pGradientStops.push_back(g2);
	newSwatch.pGradientStops.push_back(g3);

	m_pTrans->m_Swatches.push_back(newSwatch);

	m_iActiveSwatchIndex = int(m_pTrans->m_Swatches.size()-1);
	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	emit SwatchChange();
}

void Q2DTransferFunction::DeleteSwatch(){
	if (m_iActiveSwatchIndex != -1) {
		m_pTrans->m_Swatches.erase(m_pTrans->m_Swatches.begin()+m_iActiveSwatchIndex);
		
		m_iActiveSwatchIndex = min(m_iActiveSwatchIndex, int(m_pTrans->m_Swatches.size()-1));
		m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
		emit SwatchChange();
	}
}

void Q2DTransferFunction::UpSwatch(){
	if (m_iActiveSwatchIndex > 0) {
		TFPolygon tmp = m_pTrans->m_Swatches[m_iActiveSwatchIndex-1];
		m_pTrans->m_Swatches[m_iActiveSwatchIndex-1] = m_pTrans->m_Swatches[m_iActiveSwatchIndex];
		m_pTrans->m_Swatches[m_iActiveSwatchIndex] = tmp;

		m_iActiveSwatchIndex--;
		m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
		emit SwatchChange();
	}
}

void Q2DTransferFunction::DownSwatch(){
	if (m_iActiveSwatchIndex >= 0 && m_iActiveSwatchIndex < int(m_pTrans->m_Swatches.size()-1)) {
		TFPolygon tmp = m_pTrans->m_Swatches[m_iActiveSwatchIndex+1];
		m_pTrans->m_Swatches[m_iActiveSwatchIndex+1] = m_pTrans->m_Swatches[m_iActiveSwatchIndex];
		m_pTrans->m_Swatches[m_iActiveSwatchIndex] = tmp;
		
		m_iActiveSwatchIndex++;
		m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
		emit SwatchChange();
	}
}


void Q2DTransferFunction::AddGradient(GradientStop stop) {
	for (std::vector< GradientStop >::iterator i = m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.begin();i<m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.end();i++) {
		if (i->first > stop.first) {
			m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.insert(i, stop);
			return;
		}
	}
	m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.push_back(stop);
	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	update();
}

void Q2DTransferFunction::DeleteGradient(unsigned int i) {
	m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.erase(m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops.begin()+i);
	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	update();
}

void Q2DTransferFunction::SetGradient(unsigned int i, GradientStop stop) {
	m_pTrans->m_Swatches[m_iActiveSwatchIndex].pGradientStops[i] = stop;
	m_MasterController.MemMan()->Changed2DTrans(NULL, m_pTrans);
	update();
}
