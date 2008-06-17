#include "ImageVis3D.h"

#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>

MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags)
{
	setupUi(this);

	LoadWorkspace("DefaultWorkspace.wsp");
}

MainWindow::~MainWindow()
{

}

// ******************************************
// Workspace
// ******************************************

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

	QDockWidgetStateList stateList;

	stateList.Add(QDockWidgetState(dockWidget_Tools, actionTools, Qt::LeftDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_Filters,actionFilters, Qt::LeftDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_History, actionHistory, Qt::LeftDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_Information, actionInformation, Qt::LeftDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_Recorder,actionRecorder, Qt::LeftDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_1DTrans,action1D_Transfer_Function_Editor, Qt::BottomDockWidgetArea));
	stateList.Add(QDockWidgetState(dockWidget_2DTrans,action2D_Transfer_Function_Editor, Qt::BottomDockWidgetArea));

	if (stateList.LoadFromFile(strFilename)) {
		m_Workspace = stateList;
		ApplyWorkspace();
	}
}

void MainWindow::SaveWorkspace(QString strFilename) {
	
}


void MainWindow::ApplyWorkspace() {
	m_Workspace.Apply();
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
