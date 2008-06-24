#include "BrowseData.h"

#include <tools/DICOM/DICOMParser.h>
#include <QtGui/QRadioButton>
#include <QtCore/QFileInfo>


BrowseData::BrowseData(QString strDir, QWidget* parent, Qt::WindowFlags flags) : 
	m_strDir(strDir),
	m_strFilename("")
{
	setupUi(this);

	FillTable();

	// TODO: add actions to set the correct filename
	m_strFilename = "DICOM Dataset";

}

BrowseData::~BrowseData(void)
{
}


void BrowseData::FillTable()
{
	DICOMParser p;
	p.GetDirInfo(m_strDir.toStdString());


	for (unsigned int iStackID = 0;iStackID < p.m_DICOMstacks.size();iStackID++) {
		QRadioButton *pDICOMElement;
		
		unsigned iElemCount = p.m_DICOMstacks[iStackID].m_Elements.size();

		QString desc = tr(" %1 \n Acquired at %2 %3 using %4\n Size: %5 x %6 x %7\n")
			.arg(p.m_DICOMstacks[iStackID].m_strDesc.c_str())
			.arg(p.m_DICOMstacks[iStackID].m_strAcquDate.c_str())
			.arg(p.m_DICOMstacks[iStackID].m_strAcquTime.c_str())
			.arg(p.m_DICOMstacks[iStackID].m_strModality.c_str())
			.arg(p.m_DICOMstacks[iStackID].m_ivSize.x)
			.arg(p.m_DICOMstacks[iStackID].m_ivSize.y)
			.arg(p.m_DICOMstacks[iStackID].m_ivSize.z*iElemCount);

		pDICOMElement = new QRadioButton(frame);
		pDICOMElement->setObjectName(QString::fromUtf8("pDICOMElement"));
		pDICOMElement->setMinimumSize(QSize(0, 80));
		pDICOMElement->setText(desc);
		pDICOMElement->setChecked(iStackID==0);
		QIcon icon;

		QImage image(p.m_DICOMstacks[iStackID].m_ivSize.x, p.m_DICOMstacks[iStackID].m_ivSize.y, QImage::Format_RGB888);

		void* pData = NULL;
		p.m_DICOMstacks[iStackID].m_Elements[iElemCount/2].GetData(&pData);

		if (p.m_DICOMstacks[iStackID].m_iComponentCount == 1) {
			switch (p.m_DICOMstacks[iStackID].m_iAllocated) {
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
									unsigned char value = (unsigned char)(255.0f*float(pShortData[i])/float((p.m_DICOMstacks[iStackID].m_iStored)));
									image.setPixel(x,y, qRgb(value,value,value));
									i++;
								}
						   } break;
				default  : break; // TODO: handle other bitwith data
			}
		} else {
			// TODO: handle color data
		}

		delete [] pData;

		icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::Off);
		pDICOMElement->setIcon(icon);
		pDICOMElement->setIconSize(QSize(80, 80));

		verticalLayout_DICOM->addWidget(pDICOMElement);
	}

}

