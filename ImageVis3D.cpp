#include "ImageVis3D.h"

#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>

#include <fstream>
#include <string>

using namespace std;

MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags), m_strCurrentWorkspaceFilename("")
{
	setupUi(this);

	SetupWorkspaceMenu();

	LoadGeometry("Default.geo");
	LoadWorkspace("Default.wsp");
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

void MainWindow::LoadGeometry(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	settings.beginGroup("Geometry");
	restoreGeometry( settings.value("MainWinGeometry").toByteArray() ); 
	settings.endGroup();

	m_strCurrentWorkspaceFilename = strFilename;
}

void MainWindow::SaveGeometry(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

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

void MainWindow::LoadWorkspace(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	settings.beginGroup("Geometry");
	restoreState( settings.value("DockGeometry").toByteArray() ); 
	settings.endGroup();

	m_strCurrentWorkspaceFilename = strFilename;
}

void MainWindow::SaveWorkspace(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

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
	QString fileName = QFileDialog::getOpenFileName(this, "Load Dataset", ".", "All known Files (*.dat *.nrrd *.uvf);;QVis Data (*.dat);;Nearly Raw Raster Data (*.nrrd);;Universal Volume Format (*.uvf)");
	if (!fileName.isEmpty()) {
		RenderWindow *renderWin = CreateNewRenderWindow();
		renderWin->show();
	}
}

void MainWindow::LoadDirectory() {
	QString fileName = QFileDialog::getExistingDirectory(this, "Load Dataset from Directory");
	if (!fileName.isEmpty()) {
		RenderWindow *renderWin = CreateNewRenderWindow();
		renderWin->show();
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
