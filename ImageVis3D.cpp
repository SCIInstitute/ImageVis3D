#include "ImageVis3D.h"
#include "BrowseData.h"

#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>

#include <fstream>
#include <string>

using namespace std;

MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags), m_strCurrentWorkspaceFilename("")
{
	setupUi(this);

	SetupWorkspaceMenu();

	LoadGeometry("Default.geo", true);
	LoadWorkspace("Default.wsp", true);
	
	UpdateMRUActions();
}

MainWindow::~MainWindow()
{

}

// ******************************************
// Geometry
// ******************************************

void MainWindow::LoadGeometry() {
	QString fileName = QFileDialog::getOpenFileName(this, "Load Geometry", ".", "Geometry Files (*.geo)");
	if (!fileName.isEmpty()) {
		LoadGeometry(fileName);
	}
}

void MainWindow::SaveGeometry() {
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Geometry", ".", "Geometry Files (*.geo)");
	if (!fileName.isEmpty()) {
		SaveGeometry(fileName);
	}
}

void MainWindow::LoadGeometry(QString strFilename, bool bSilentFail) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	settings.beginGroup("Geometry");
	bool bOK = restoreGeometry( settings.value("MainWinGeometry").toByteArray() ); 
	settings.endGroup();

	if (!bSilentFail && !bOK) {
		QString msg = tr("Error reading geometry file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return;
	}

	m_strCurrentWorkspaceFilename = strFilename;
}

void MainWindow::SaveGeometry(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	if (!settings.isWritable()) {
		QString msg = tr("Error saving geometry file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return;
	}

	settings.beginGroup("Geometry");
	settings.setValue("MainWinGeometry", this->saveGeometry() ); 
	settings.endGroup(); 	
}


// ******************************************
// Workspace
// ******************************************

void MainWindow::SetupWorkspaceMenu() {
	menu_Workspace->addAction(dockWidget_Tools->toggleViewAction());
	menu_Workspace->addAction(dockWidget_Filters->toggleViewAction());
	menu_Workspace->addSeparator();
	menu_Workspace->addAction(dockWidget_History->toggleViewAction());
	menu_Workspace->addAction(dockWidget_Information->toggleViewAction());
	menu_Workspace->addAction(dockWidget_Recorder->toggleViewAction());
	menu_Workspace->addSeparator();
	menu_Workspace->addAction(dockWidget_1DTrans->toggleViewAction());
	menu_Workspace->addAction(dockWidget_2DTrans->toggleViewAction());
}

void MainWindow::LoadWorkspace() {
	QString fileName = QFileDialog::getOpenFileName(this, "Load Workspace", ".", "Workspace Files (*.wsp)");
	if (!fileName.isEmpty()) {
		LoadWorkspace(fileName);
	}
}

void MainWindow::SaveWorkspace() {
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Workspace", ".", "Workspace Files (*.wsp)");
	if (!fileName.isEmpty()) {
		SaveWorkspace(fileName);
	}
}

void MainWindow::LoadWorkspace(QString strFilename, bool bSilentFail) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	settings.beginGroup("Geometry");
	bool bOK = restoreState( settings.value("DockGeometry").toByteArray() ); 
	settings.endGroup();

	if (!bSilentFail && !bOK) {
		QString msg = tr("Error reading workspace file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return;
	}

	m_strCurrentWorkspaceFilename = strFilename;
}

void MainWindow::SaveWorkspace(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	if (!settings.isWritable()) {
		QString msg = tr("Error saving workspace file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return;
	}

	settings.beginGroup("Geometry");
	settings.setValue("DockGeometry", this->saveState() ); 
	settings.endGroup(); 	

}


void MainWindow::ApplyWorkspace() {
	if (!m_strCurrentWorkspaceFilename.isEmpty())
		LoadWorkspace(m_strCurrentWorkspaceFilename);
}


// ******************************************
// Render Windows
// ******************************************

void MainWindow::CloneCurrentView() {
	 RenderWindow *renderWin = CreateNewRenderWindow();
	 renderWin->show();
}

void MainWindow::LoadDataset() {
	LoadDataset(QFileDialog::getOpenFileName(this, "Load Dataset", ".", "All known Files (*.dat *.nrrd *.uvf);;QVis Data (*.dat);;Nearly Raw Raster Data (*.nrrd);;Universal Volume Format (*.uvf)"));
}

void MainWindow::LoadDataset(QString fileName) {
	if (!fileName.isEmpty()) {
		RenderWindow *renderWin = CreateNewRenderWindow();
		renderWin->show();

		AddFileToMRUList(fileName);
	}
}


void MainWindow::LoadDirectory() {
	QString directoryName = QFileDialog::getExistingDirectory(this, "Load Dataset from Directory");
	if (!directoryName.isEmpty()) {
	
		QString fileName;

		BrowseData browseDataDialog(directoryName, this);

		if (browseDataDialog.exec() == QDialog::Accepted) {
			LoadDataset(browseDataDialog.GetFileName());
		}
	}
}


RenderWindow* MainWindow::CreateNewRenderWindow()
 {
     RenderWindow *renderWin = new RenderWindow;
     mdiArea->addSubWindow(renderWin);

     return renderWin;
 }


void MainWindow::ToggleRenderWindowView1x3() {
	RenderWindow* win = GetActiveRenderWindow();
	if (win) win->ToggleRenderWindowView1x3();
}

void MainWindow::ToggleRenderWindowView2x2() {
	RenderWindow* win = GetActiveRenderWindow();
	if (win) win->ToggleRenderWindowView2x2();
}

void MainWindow::ToggleRenderWindowViewSingle() {
	RenderWindow* win = GetActiveRenderWindow();
	if (win) win->ToggleRenderWindowViewSingle();
}

RenderWindow* MainWindow::GetActiveRenderWindow()
{
	if (QMdiSubWindow* activeSubWindow = mdiArea->activeSubWindow())
		return qobject_cast<RenderWindow*>(activeSubWindow->widget());
	else
		return NULL;
}


// ******************************************
// 1D Transfer Function Dock
// ******************************************

void MainWindow::Transfer1DCBClicked() {
	radioButton_User->setChecked(true);
}

void MainWindow::Transfer1DRadioClicked() {
	// determine current CB state
	unsigned int iState;
	if (radioButton_User->isChecked()) iState = 0;
		else if (radioButton_Luminance->isChecked()) iState = 1;
			else iState = 2;

	// if were in usermode exit
	if (iState == 0) return;

	// apply checks according to current state
	checkBox_Red->setChecked(true);	
	checkBox_Green->setChecked(true);	
	checkBox_Blue->setChecked(true);	
	checkBox_Alpha->setChecked(iState==2);
}

void MainWindow::Use1DTrans() {
	checkBox_Use2DTrans->setChecked(!checkBox_Use1DTrans->isChecked());
}

void MainWindow::Use2DTrans() {
	checkBox_Use1DTrans->setChecked(!checkBox_Use2DTrans->isChecked());
}

// ******************************************
// Recent Files
// ******************************************

 void MainWindow::ClearMRUList()
 {
     QSettings settings("ImageVis3D", "Recent Files");
     QStringList files;
	 files.clear();
	 settings.setValue("recentFileList", files);

     UpdateMRUActions();
 }

 void MainWindow::AddFileToMRUList(const QString &fileName)
 {
     QSettings settings("ImageVis3D", "Recent Files");
     QStringList files = settings.value("recentFileList").toStringList();

	 files.removeAll(fileName);
     files.prepend(fileName);
     while (files.size() > ms_iMaxRecentFiles) files.removeLast();

     settings.setValue("recentFileList", files);

     UpdateMRUActions();
 }

 void MainWindow::UpdateMRUActions()
 {
     QSettings settings("ImageVis3D", "Recent Files");
     QStringList files = settings.value("recentFileList").toStringList();

     int numRecentFiles = qMin(files.size(), (int)ms_iMaxRecentFiles);

     for (int i = 0; i < numRecentFiles; ++i) {
         QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
         m_recentFileActs[i]->setText(text);
         m_recentFileActs[i]->setData(files[i]);
         m_recentFileActs[i]->setVisible(true);
     }
     for (int j = numRecentFiles; j < ms_iMaxRecentFiles; ++j) m_recentFileActs[j]->setVisible(false);
 }

QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::setupUi(QMainWindow *MainWindow) {

	Ui_MainWindow::setupUi(MainWindow);

	for (int i = 0; i < ms_iMaxRecentFiles; ++i) {
		m_recentFileActs[i] = new QAction(this);
		m_recentFileActs[i]->setVisible(false);
		connect(m_recentFileActs[i], SIGNAL(triggered()),this, SLOT(OpenRecentFile()));
		menuLast_Used_Projects->addAction(m_recentFileActs[i]);
	}
}

void MainWindow::OpenRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) LoadDataset(action->data().toString());
}