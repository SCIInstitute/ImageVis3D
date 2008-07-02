#pragma once

#ifndef IMAGEVIS3D_H
#define IMAGEVIS3D_H

#include "ui_ImageVis3D.h"
#include "RenderWindow.h"
#include "Q1DTransferFunction.h"


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
		void UseIso();

		void LoadWorkspace();
		void SaveWorkspace();
		void ApplyWorkspace();

		void LoadGeometry();
		void SaveGeometry();
		void OpenRecentFile();

		void ClearMRUList();

		void UpdateMenus();

		void SaveDataset();
		void Load1DTrans();
		void Save1DTrans();
		void Load2DTrans();
		void Save2DTrans();

		void EditViewLocks();
		void EditRenderLocks();
		void EditToolsLocks();
		void EditFiltersLocks();


	private :
		QString					m_strCurrentWorkspaceFilename;
		Q1DTransferFunction*	m_1DTransferFunction;

		RenderWindow* CreateNewRenderWindow(QString dataset);
		RenderWindow* GetActiveRenderWindow();

		void SetupWorkspaceMenu();
		void LoadWorkspace(QString strFilename, bool bSilentFail = false);
		void SaveWorkspace(QString strFilename);

		void LoadGeometry(QString strFilename, bool bSilentFail = false);
		void SaveGeometry(QString strFilename);

		QString strippedName(const QString &fullFileName);
		static const unsigned int ms_iMaxRecentFiles = 5;
		QAction *m_recentFileActs[ms_iMaxRecentFiles];
		void UpdateMRUActions();
		void AddFileToMRUList(const QString &fileName);

		void setupUi(QMainWindow *MainWindow);
		
		void LoadDataset(QString fileName);
		
};

#endif // IMAGEVIS3D_H
