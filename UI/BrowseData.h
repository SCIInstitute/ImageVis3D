#pragma once

#ifndef BROWSEDATA_H
#define BROWSEDATA_H

#include "AutoGen/ui_BrowseData.h"
#include "RenderWindow.h"

class BrowseData : public QDialog, protected Ui_BrowseData
{
	Q_OBJECT
	public:
		BrowseData(QDialog* pleaseWaitDialog, QString strDir, QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~BrowseData() {}

		QString GetFileName() {return m_strFilename;}

		bool DataFound() {return m_bDataFound;}

	private:
		bool	m_bDataFound;
		QString m_strDir;
		QString m_strFilename;


		bool FillTable(QDialog* pleaseWaitDialog);

		virtual void showEvent ( QShowEvent * event );
};


#endif // BROWSEDATA_H
