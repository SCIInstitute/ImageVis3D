#pragma once

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "ui_RenderWindow.h"
#include <QtGui/QListWidget>


class RenderWindow : public QDialog, protected Ui_RenderWindow
{
	Q_OBJECT
	public:
		RenderWindow(QString dataset, QListWidget *listWidget_Lock, unsigned int iCounter, QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~RenderWindow();

		void ToggleRenderWindowView1x3();
		void ToggleRenderWindowView2x2();
		void ToggleRenderWindowViewSingle();
		
		QString GetDataset() {return m_strDataset;}
	
	protected:
		void closeEvent(QCloseEvent *event);

	private :
		QPixmap m_Pixmap1x3;
		QPixmap m_Pixmap2x2;
		QPixmap m_PixmapSingle;
		QString m_strDataset;
		QString m_strID;
		QListWidget *m_listWidget_Lock;

		void LoadImages();

};

#endif // RENDERWINDOW_H
