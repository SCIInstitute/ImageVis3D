#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include "ui_ImageVis3D.h"
#include "TransDialog1D.h"
#include "TransDialog2D.h"
#include "RenderWindow.h"


class MainWindow : public QMainWindow, protected Ui_MainWindow
{
	Q_OBJECT
	public:
		MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~MainWindow();
	
	protected slots:
		void Show1DTransferFunction();
		void Show2DTransferFunction();
		void LoadDataset();
		void CloneCurrentView();

		void ToggleRenderWindowView1x3();
		void ToggleRenderWindowView2x2();
		void ToggleRenderWindowViewSingle();

	private :
		TransDialog1D m_TransDialog1D;
		TransDialog2D m_TransDialog2D;

		RenderWindow* CreateNewRenderWindow();
		RenderWindow* GetActiveRenderWindow();

};

#endif 