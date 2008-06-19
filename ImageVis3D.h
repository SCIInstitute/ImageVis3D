#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include "ui_ImageVis3D.h"
#include "RenderWindow.h"


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

		void Use1DTrans();
		void Use2DTrans();

		void LoadWorkspace();
		void SaveWorkspace();
		void ApplyWorkspace();

		void LoadGeometry();
		void SaveGeometry();

	private :
		QString m_strCurrentWorkspaceFilename;

		RenderWindow* CreateNewRenderWindow();
		RenderWindow* GetActiveRenderWindow();

		void SetupWorkspaceMenu();
		void LoadWorkspace(QString strFilename);
		void SaveWorkspace(QString strFilename);

		void LoadGeometry(QString strFilename);
		void SaveGeometry(QString strFilename);
		
};

#endif // IMAGEVIS3D_H
