#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include "ui_ImageVis3D.h"
#include "RenderWindow.h"
#include "QDockWidgetStateList.h"


class MainWindow : public QMainWindow, protected Ui_MainWindow
{
	Q_OBJECT
	public:
		MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~MainWindow();
	
	protected slots:
		void LoadDataset();
		void LoadDirectory();
		void CloneCurrentView();

		void ToggleRenderWindowView1x3();
		void ToggleRenderWindowView2x2();
		void ToggleRenderWindowViewSingle();

		void Transfer1DCBClicked();
		void Transfer1DRadioClicked();

		void LoadWorkspace();
		void SaveWorkspace();
		void ApplyWorkspace();

	private :
		RenderWindow* CreateNewRenderWindow();
		RenderWindow* GetActiveRenderWindow();

		QDockWidgetStateList m_Workspace;
		void LoadWorkspace(QString strFilename);
		void SaveWorkspace(QString strFilename);
		
};

#endif 