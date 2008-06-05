#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include "ui_ImageVis3D.h"
#include "TransDialog1D.h"
#include "TransDialog2D.h"


class MainWindow : public QMainWindow, protected Ui_MainWindow
{
	Q_OBJECT
	public:
		MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~MainWindow();
	
	protected slots:
		void Show1DTransferFunction();
		void Show2DTransferFunction();
		void HideAllTools();
		void HideAllFilters();
		void HideVisualizationWindows();

	private :
		TransDialog1D m_TransDialog1D;
		TransDialog2D m_TransDialog2D;

};

#endif 