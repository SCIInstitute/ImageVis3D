#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "ui_RenderWindow.h"


class RenderWindow : public QDialog, protected Ui_RenderWindow
{
	Q_OBJECT
	public:
		RenderWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~RenderWindow();

		void ToggleRenderWindowView1x3();
		void ToggleRenderWindowView2x2();
		void ToggleRenderWindowViewSingle();

private :
		void LoadImages();
		QPixmap m_Pixmap1x3;
		QPixmap m_Pixmap2x2;
		QPixmap m_PixmapSingle;


};

#endif // RENDERWINDOW_H
