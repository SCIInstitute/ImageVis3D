#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "ui_RenderWindow.h"


class RenderWindow : public QDialog, protected Ui_RenderWindow
{
	Q_OBJECT
	public:
		RenderWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~RenderWindow();

	protected slots :

	private :
		void LoadMockupImage();

};

#endif // RENDERWINDOW_H
