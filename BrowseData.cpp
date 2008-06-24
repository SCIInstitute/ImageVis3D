#include "BrowseData.h"

#include "QDataRadioButton.h"
#include <QtCore/QFileInfo>


BrowseData::BrowseData(QDialog* pleaseWaitDialog, QString strDir, QWidget* parent, Qt::WindowFlags flags) : 
	m_strDir(strDir),
	m_strFilename("")
{
	setupUi(this);

	FillTable(pleaseWaitDialog);

	// TODO: add actions to set the correct filename
	m_strFilename = "DICOM Dataset";
}


void BrowseData::showEvent ( QShowEvent * event ) {
}


void BrowseData::FillTable(QDialog* pleaseWaitDialog)
{
	DICOMParser p;
	p.GetDirInfo(m_strDir.toStdString());

	for (unsigned int iStackID = 0;iStackID < p.m_DICOMstacks.size();iStackID++) {
		QDataRadioButton *pDICOMElement;
		pDICOMElement = new QDataRadioButton(p.m_DICOMstacks[iStackID],frame);
		pDICOMElement->setMinimumSize(QSize(0, 80));
		pDICOMElement->setChecked(iStackID==0);
		pDICOMElement->setIconSize(QSize(80, 80));
		verticalLayout_DICOM->addWidget(pDICOMElement);
	}

	if (pleaseWaitDialog != NULL) pleaseWaitDialog->close();
}

