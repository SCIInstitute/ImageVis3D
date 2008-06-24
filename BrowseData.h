#pragma once

#ifndef BROWSEDATA_H
#define BROWSEDATA_H

#include "ui_BrowseData.h"
#include "RenderWindow.h"

class BrowseData : public QDialog, protected Ui_BrowseData
{
	Q_OBJECT
	public:
		BrowseData(QDialog* pleaseWaitDialog, QString strDir, QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~BrowseData() {}

		QString GetFileName() {return m_strFilename;}

	private:
		QString m_strDir;
		QString m_strFilename;

		void FillTable(QDialog* pleaseWaitDialog);

		virtual void showEvent ( QShowEvent * event );
};


#endif // BROWSEDATA_H
