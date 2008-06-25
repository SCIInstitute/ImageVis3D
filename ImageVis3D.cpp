#include "ImageVis3D.h"
#include "BrowseData.h"

#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>

#include "PleaseWait.h"

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
	UpdateMenus();
}

MainWindow::~MainWindow()
{

}

void MainWindow::SaveDataset(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Dataset", ".", "Nearly Raw Raster Data (*.nrrd);;QVis Data (*.dat);;Universal Volume Format (*.uvf)");
}

void MainWindow::Load1DTrans(){
	QString fileName = QFileDialog::getOpenFileName(this, "Load 1D Transferfunction", ".", "1D Transferfunction File (*.1dt)");
}

void MainWindow::Save1DTrans(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save 1D Transferfunction", ".", "1D Transferfunction File (*.1dt)");
}

void MainWindow::Load2DTrans(){
	QString fileName = QFileDialog::getOpenFileName(this, "Load 2D Transferfunction", ".", "2D Transferfunction File (*.2dt)");
}

void MainWindow::Save2DTrans(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save 2D Transferfunction", ".", "2D Transferfunction File (*.2dt)");
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
	menu_Workspace->addAction(dockWidget_LockOptions->toggleViewAction());
	menu_Workspace->addAction(dockWidget_RenderOptions->toggleViewAction());
	menu_Workspace->addSeparator();
	menu_Workspace->addAction(dockWidget_1DTrans->toggleViewAction());
	menu_Workspace->addAction(dockWidget_2DTrans->toggleViewAction());
	menu_Workspace->addAction(dockWidget_IsoSurface->toggleViewAction());
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
	 RenderWindow *renderWin = CreateNewRenderWindow(GetActiveRenderWindow()->GetDataset());
	 renderWin->show();
}

void MainWindow::LoadDataset() {
	LoadDataset(QFileDialog::getOpenFileName(this, "Load Dataset", ".", "All known Files (*.dat *.nrrd *.uvf);;QVis Data (*.dat);;Nearly Raw Raster Data (*.nrrd);;Universal Volume Format (*.uvf)"));
}

void MainWindow::LoadDataset(QString fileName) {
	if (!fileName.isEmpty()) {
		RenderWindow *renderWin = CreateNewRenderWindow(fileName);
		renderWin->show();

		AddFileToMRUList(fileName);
	}
}

void MainWindow::LoadDirectory() {
	PleaseWaitDialog pleaseWait(this);

	QString directoryName = QFileDialog::getExistingDirectory(this, "Load Dataset from Directory");

	if (!directoryName.isEmpty()) {
		pleaseWait.SetText("Scanning directory for DICOM files, please wait  ...");

		QString fileName;
		BrowseData browseDataDialog((QDialog*)&pleaseWait,directoryName, this);

		if (browseDataDialog.DataFound()) {
			if (browseDataDialog.exec() == QDialog::Accepted) {
				LoadDataset(browseDataDialog.GetFileName());
			}
		} else {
			QString msg = tr("Error no valid DICOM files in directory %1 found.").arg(directoryName);
			QMessageBox::information(this, tr("Problem"), msg);
		}
	}
}


RenderWindow* MainWindow::CreateNewRenderWindow(QString dataset)
 {
	 static unsigned int iCounter = 0;
	 
     RenderWindow *renderWin = new RenderWindow(dataset, listWidget_Lock, iCounter++);
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

void MainWindow::UpdateMenus() {
	bool bHasMdiChild = (GetActiveRenderWindow() != 0);
	actionSave_Dataset->setEnabled(bHasMdiChild);

    actionGo_Fullscreen->setEnabled(bHasMdiChild);
	actionCascade->setEnabled(bHasMdiChild);
    actionTile->setEnabled(bHasMdiChild);
    actionNext->setEnabled(bHasMdiChild);
    actionPrevious->setEnabled(bHasMdiChild);
    action2_x_2_View->setEnabled(bHasMdiChild);
    action1_x_3_View->setEnabled(bHasMdiChild);
    actionSinge_View->setEnabled(bHasMdiChild);
    actionCloneCurrentView->setEnabled(bHasMdiChild);

	actionBox->setEnabled(bHasMdiChild);
    actionPoly_Line->setEnabled(bHasMdiChild);
    actionSelect_All->setEnabled(bHasMdiChild);
    actionDelete_Selection->setEnabled(bHasMdiChild);
    actionInvert_Selection->setEnabled(bHasMdiChild);
    actionStastistcs->setEnabled(bHasMdiChild);
    actionUndo->setEnabled(bHasMdiChild);
    actionRedo->setEnabled(bHasMdiChild);	
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
	checkBox_Use2DTrans->setChecked(false);
	checkBox_Use2DTrans->setEnabled(true);
	checkBox_UseIso->setChecked(false);
	checkBox_UseIso->setEnabled(true);
	checkBox_Use1DTrans->setEnabled(false);
	checkBox_Use1DTrans->setChecked(true);
	radioButton_1DTrans->setChecked(true);
}

void MainWindow::Use2DTrans() {
	checkBox_Use1DTrans->setChecked(false);
	checkBox_Use1DTrans->setEnabled(true);
	checkBox_UseIso->setChecked(false);
	checkBox_UseIso->setEnabled(true);
	checkBox_Use2DTrans->setEnabled(false);
	checkBox_Use2DTrans->setChecked(true);
	radioButton_2DTrans->setChecked(true);
}

void MainWindow::UseIso() {
	checkBox_Use2DTrans->setChecked(false);
	checkBox_Use2DTrans->setEnabled(true);
	checkBox_Use1DTrans->setChecked(false);
	checkBox_Use1DTrans->setEnabled(true);
	checkBox_UseIso->setEnabled(false);
	checkBox_UseIso->setChecked(true);
	radioButton_Iso->setChecked(true);
}


// ******************************************
// Locks
// ******************************************

void MainWindow::EditViewLocks() {
	pushButton_RelativeLock->setEnabled(true);	
}

void MainWindow::EditRenderLocks() {
	pushButton_RelativeLock->setEnabled(false);
}

void MainWindow::EditToolsLocks() {
	pushButton_RelativeLock->setEnabled(false);
}

void MainWindow::EditFiltersLocks() {
	pushButton_RelativeLock->setEnabled(false);
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
     while ((unsigned int)(files.size()) > ms_iMaxRecentFiles) files.removeLast();

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
     for (unsigned int j = numRecentFiles; j < ms_iMaxRecentFiles; ++j) m_recentFileActs[j]->setVisible(false);
 }

QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::setupUi(QMainWindow *MainWindow) {

	Ui_MainWindow::setupUi(MainWindow);

	for (unsigned int i = 0; i < ms_iMaxRecentFiles; ++i) {
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
