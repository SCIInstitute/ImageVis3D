#include "QDataRadioButton.h"
#include <QtGui/QMouseEvent>

QDataRadioButton::QDataRadioButton(DICOMStackInfo stack, QWidget *parent) : 
	QRadioButton(parent), 
	m_iCurrentImage((unsigned int)(-1)),  
	m_stackInfo(stack) 
{
		SetupInfo();
}
QDataRadioButton::QDataRadioButton(DICOMStackInfo stack, const QString &text, QWidget *parent) :
	QRadioButton(text, parent), 
	m_iCurrentImage((unsigned int)(-1)),  
	m_stackInfo(stack) 
{
	SetupInfo();
}


void QDataRadioButton::leaveEvent ( QEvent * event ) {
	QRadioButton::leaveEvent(event);

	SetStackImage(m_stackInfo.m_Elements.size()/2);
}

void QDataRadioButton::mouseMoveEvent(QMouseEvent *event){
	QRadioButton::mouseMoveEvent(event);

	unsigned int i = (unsigned int)(float(event->x())/float(width())*m_stackInfo.m_Elements.size());

	SetStackImage(i);
}

void QDataRadioButton::SetStackImage(unsigned int i) {

	if (m_iCurrentImage == i) return;
	
	m_iCurrentImage = i;

	QIcon icon;
	QImage image(m_stackInfo.m_ivSize.x, m_stackInfo.m_ivSize.y, QImage::Format_RGB888);

	void* pData = NULL;
	m_stackInfo.m_Elements[i].GetData(&pData);

	if (m_stackInfo.m_iComponentCount == 1) {
		switch (m_stackInfo.m_iAllocated) {
			case 8  :{
						unsigned int i = 0;
						unsigned char* pCharData = (unsigned char*)pData;
						for (int y = 0;y<image.height();y++) 
							for (int x = 0;x<image.width();x++) {
								image.setPixel(x,y, qRgb(pCharData[i],pCharData[i],pCharData[i]));
								i++;
							}
					   } break;
			case 16 : {
						unsigned int i = 0;
						unsigned short* pShortData = (unsigned short*)pData;
						for (int y = 0;y<image.height();y++) 
							for (int x = 0;x<image.width();x++) {
								unsigned char value = (unsigned char)(255.0f*float(pShortData[i])/float((m_stackInfo.m_iStored)));
								image.setPixel(x,y, qRgb(value,value,value));
								i++;
							}
					   } break;
			default  : break; // TODO: handle other bitwith data
		}
	} else {
		// TODO: handle color data
	}

	delete [] (char*)pData;


	icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::On);
	setIcon(icon);
	update();
}


void QDataRadioButton::SetupInfo() {
	setMouseTracking(true); 

	unsigned iElemCount = m_stackInfo.m_Elements.size();

	QString desc = tr(" %1 \n Acquired at %2 %3 using %4\n Size: %5 x %6 x %7\n")
		.arg(m_stackInfo.m_strDesc.c_str())
		.arg(m_stackInfo.m_strAcquDate.c_str())
		.arg(m_stackInfo.m_strAcquTime.c_str())
		.arg(m_stackInfo.m_strModality.c_str())
		.arg(m_stackInfo.m_ivSize.x)
		.arg(m_stackInfo.m_ivSize.y)
		.arg(m_stackInfo.m_ivSize.z*iElemCount);

	setText(desc);

	SetStackImage(iElemCount/2);
}
