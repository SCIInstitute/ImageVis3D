#include "ImageVis3D.h"
#include "BrowseData.h"

#include <QtCore/QTimer>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>
#include <QtGui/QColorDialog>

#include "PleaseWait.h"

#include <fstream>
#include <string>
#include <Basics/SysTools.h>

using namespace std;


MainWindow::MainWindow(MasterController& masterController, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) :
	QMainWindow(parent, flags),
	m_MasterController(masterController),
	m_strCurrentWorkspaceFilename(""),
	m_ActiveRenderWin(NULL)
{
	setupUi(this);

	m_DebugOut = new QTOut(listWidget_3);
	m_MasterController.SetDebugOut(m_DebugOut);

	SetupWorkspaceMenu();

	LoadGeometry("Default.geo", true);
	LoadWorkspace("Default.wsp", true);
	
	UpdateMRUActions();
	UpdateMenus();

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(CheckForRedraw()));
	timer->start(20);
}

MainWindow::~MainWindow()
{
	m_MasterController.RemoveDebugOut(m_DebugOut);
	delete m_DebugOut;
}

void MainWindow::SaveDataset(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Dataset", ".", "Nearly Raw Raster Data (*.nrrd);;QVis Data (*.dat);;Universal Volume Format (*.uvf)");
}

void MainWindow::Load1DTrans(){
	QString fileName = QFileDialog::getOpenFileName(this, "Load 1D Transferfunction", ".", "1D Transferfunction File (*.1dt)");

	if (fileName != "") {
		m_1DTransferFunction->LoadFromFile(fileName);
	}
}

void MainWindow::Save1DTrans(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save 1D Transferfunction", ".", "1D Transferfunction File (*.1dt)");
	if (fileName != "") {
		m_1DTransferFunction->SaveToFile(fileName);
	}
}

void MainWindow::Copy1DTransTo2DTrans() {
	m_2DTransferFunction->Set1DTrans(m_1DTransferFunction->GetTrans());
}

void MainWindow::SwatchesChanged() {
	listWidget_Swatches->clear();
	
	for (size_t i = 0;i<m_2DTransferFunction->GetSwatchCount();i++) {
		size_t iSize = m_2DTransferFunction->GetSwatchSize(i);

		QString msg;
		switch (iSize) {
			case  3  : msg = "Triangle";		break;
			case  4  : msg = "Quadrilateral";	break;
			case  5  : msg = "Pentagon";		break;
			case  6  : msg = "Hexagon";			break;
			case  7  : msg = "Heptagon";		break;
			case  8  : msg = "Octagon";			break;
			case  9  : msg = "Nonagon";			break;
			case 10  : msg = "Decagon";			break;
			case 11  : msg = "Hendecagon";		break;
			case 12  : msg = "Dodecagon";		break;
			case 13  : msg = "Triskaidecagon";	break;
			case 14  : msg = "Tetrakaidecagon";	break;
			case 15  : msg = "Pendedecagon";	break;
			case 16  : msg = "Hexdecagon";		break;
			case 17  : msg = "Heptdecagon";		break;
			case 18  : msg = "Octdecagon";		break;
			case 19  : msg = "Enneadecagon";	break;
			case 20  : msg = "Icosagon";		break;
			// at this point I am getting bored ...
			default : msg = tr("%1 - gon").arg(iSize); break;
		}

		listWidget_Swatches->addItem( msg );
	}

	int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();
	listWidget_Swatches->setCurrentRow(iCurrent);

	UpdateSwatchButtons();
	UpdateGradientBox();
}

void MainWindow::UpdateSwatchButtons() {
	int iCurrent = m_2DTransferFunction->GetActiveSwatchIndex();

	pushButton_DelPoly->setEnabled(iCurrent >= 0);
	pushButton_UpPoly->setEnabled(iCurrent > 0);
	pushButton_DownPoly->setEnabled(iCurrent < int(m_2DTransferFunction->GetSwatchCount())-1);

	UpdateGradientBox();
}

void MainWindow::UpdateGradientBox() {
	listWidget_Gradient->clear();

	if (m_2DTransferFunction->GetActiveSwatchIndex() > -1) {
	
		for (size_t i = 0;i<m_2DTransferFunction->GetGradientCount();i++) {
			GradientStop s =  m_2DTransferFunction->GetGradient(i);
			QString msg = tr("Stop at %1").arg(s.first);
			listWidget_Gradient->addItem( msg );
		}
	}

	UpdateGradientButtons();
}

void MainWindow::UpdateGradientButtons() {

	pushButton_AddStop->setEnabled(m_2DTransferFunction->GetActiveSwatchIndex() != -1);

	int iCurrent = listWidget_Gradient->currentRow();
	pushButton_DelStop->setEnabled(iCurrent >= 0);
	frame_ChooseColor->setEnabled(iCurrent >= 0);
}


void MainWindow::AddGradient() {
	bool ok;
	float f = float(QInputDialog::getDouble(this, tr("Select Gradient Stop"), tr("Stop at:"), 0.5, 0, 1, 3, &ok));

	if (!ok) return;

	GradientStop s(f, FLOATVECTOR4(1,1,1,1));
	m_2DTransferFunction->AddGradient(s);

	UpdateGradientBox();
}

void MainWindow::ChooseGradientColor() {
	GradientStop s =  m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());

	QColor color = QColorDialog::getColor(Qt::green, this);
	s.second[0] = color.red()/255.0f;
	s.second[1] = color.green()/255.0f;
	s.second[2] = color.blue()/255.0f;

	m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);
}

void MainWindow::ChooseGradientOpacity() {
	GradientStop s =  m_2DTransferFunction->GetGradient(listWidget_Gradient->currentRow());
	s.second[3] = horizontalSlider_Opacity->value()/100.0f;
	m_2DTransferFunction->SetGradient(listWidget_Gradient->currentRow(),s);
}

void MainWindow::DeleteGradient() {
	m_2DTransferFunction->DeleteGradient(listWidget_Gradient->currentRow());
	UpdateGradientBox();
}


void MainWindow::Load2DTrans(){
	QString fileName = QFileDialog::getOpenFileName(this, "Load 2D Transferfunction", ".", "2D Transferfunction File (*.2dt)");
	if (fileName != "") m_2DTransferFunction->LoadFromFile(fileName);
}

void MainWindow::Save2DTrans(){
	QString fileName = QFileDialog::getSaveFileName(this, "Save 2D Transferfunction", ".", "2D Transferfunction File (*.2dt)");
	if (fileName != "")	m_2DTransferFunction->SaveToFile(fileName);
}

// ******************************************
// Geometry
// ******************************************

bool MainWindow::LoadGeometry() {
	QString fileName = QFileDialog::getOpenFileName(this, "Load Geometry", ".", "Geometry Files (*.geo)");
	if (!fileName.isEmpty()) {
		return LoadGeometry(fileName);
	} else return false;
}

bool MainWindow::SaveGeometry() {
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Geometry", ".", "Geometry Files (*.geo)");
	if (!fileName.isEmpty()) {
		return SaveGeometry(fileName);
	} return false;
}

bool MainWindow::LoadGeometry(QString strFilename, bool bSilentFail, bool bRetryResource) {
	QSettings settings( strFilename, QSettings::IniFormat );

	settings.beginGroup("Geometry");
	bool bOK = restoreGeometry( settings.value("MainWinGeometry").toByteArray() ); 
	settings.endGroup();

	if (!bOK && bRetryResource) {
		string stdString(strFilename.toAscii());
		if (LoadGeometry(SysTools::GetFromResourceOnMac(stdString).c_str(), true, false)) {
			return true;
		}
	}

	if (!bSilentFail && !bOK) {
		QString msg = tr("Error reading geometry file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return false;
	}

	return bOK;
}

bool MainWindow::SaveGeometry(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	if (!settings.isWritable()) {
		QString msg = tr("Error saving geometry file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return false;
	}

	settings.beginGroup("Geometry");
	settings.setValue("MainWinGeometry", this->saveGeometry() ); 
	settings.endGroup();

	return true;
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

	menu_Help->addAction(dockWidget_Debug->toggleViewAction());

}

bool MainWindow::LoadWorkspace() {
	QString fileName = QFileDialog::getOpenFileName(this, "Load Workspace", ".", "Workspace Files (*.wsp)");
	if (!fileName.isEmpty()) {
		return LoadWorkspace(fileName);
	} else return false;
}

bool MainWindow::SaveWorkspace() {
	QString fileName = QFileDialog::getSaveFileName(this, "Save Current Workspace", ".", "Workspace Files (*.wsp)");
	if (!fileName.isEmpty()) {
		return SaveWorkspace(fileName);
	} else return false;
}

bool MainWindow::LoadWorkspace(QString strFilename, bool bSilentFail, bool bRetryResource) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	settings.beginGroup("Geometry");
	bool bOK = restoreState( settings.value("DockGeometry").toByteArray() ); 
	settings.endGroup();

	if (!bOK && bRetryResource) {
		string stdString(strFilename.toAscii());

		if (LoadWorkspace(SysTools::GetFromResourceOnMac(stdString).c_str(), true, false)) {
			m_strCurrentWorkspaceFilename = SysTools::GetFromResourceOnMac(stdString).c_str();
			return true;
		}
	}

	if (!bSilentFail && !bOK) {
		QString msg = tr("Error reading workspace file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return false;
	}

	m_strCurrentWorkspaceFilename = strFilename;

	return bOK;
}

bool MainWindow::SaveWorkspace(QString strFilename) {
	QSettings settings( strFilename, QSettings::IniFormat ); 

	if (!settings.isWritable()) {
		QString msg = tr("Error saving workspace file %1").arg(strFilename);
		QMessageBox::warning(this, tr("Error"), msg);
		return false;
	}

	settings.beginGroup("Geometry");
	settings.setValue("DockGeometry", this->saveState() ); 
	settings.endGroup(); 	

	return true;
}


bool MainWindow::ApplyWorkspace() {
	if (!m_strCurrentWorkspaceFilename.isEmpty())
		return LoadWorkspace(m_strCurrentWorkspaceFilename);
	else 
		return false;
}


// ******************************************
// Render Windows
// ******************************************

void MainWindow::CloneCurrentView() {
	 RenderWindow *renderWin = CreateNewRenderWindow(GetActiveRenderWindow()->GetDatasetName());
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

	RenderWindow *renderWin = new RenderWindow(m_MasterController, dataset, iCounter++, m_glShareWidget, this);
	mdiArea->addSubWindow(renderWin);
	listWidget_Lock->addItem(renderWin->GetWindowID());

	connect(renderWin, SIGNAL(WindowActive(RenderWindow*)), this, SLOT(RenderWindowActive(RenderWindow*)));
	connect(renderWin, SIGNAL(WindowClosing(RenderWindow*)), this, SLOT(RenderWindowClosing(RenderWindow*)));

	return renderWin;
}

void MainWindow::RenderWindowActive(RenderWindow* sender) {
	if (m_ActiveRenderWin != sender) {
		m_MasterController.DebugOut()->Message("MainWindow::RenderWindowActive","ACK that %s is now active", sender->GetDatasetName().toStdString().c_str());
		m_ActiveRenderWin = sender;

		m_1DTransferFunction->SetData(sender->GetRenderer()->GetDataSet()->Get1DHistogramm(), sender->GetRenderer()->Get1DTrans());
		m_1DTransferFunction->update();
		m_2DTransferFunction->SetData(sender->GetRenderer()->GetDataSet()->Get2DHistogramm(), sender->GetRenderer()->Get2DTrans());
		m_2DTransferFunction->update();

		ERenderMode e = m_ActiveRenderWin->GetRenderer()->GetRendermode();

		switch (e) {
			case RM_1DTRANS    : Use1DTrans(); break;
			case RM_2DTRANS    : Use2DTrans(); break;
			case RM_ISOSURFACE : UseIso(); break;
			default : m_MasterController.DebugOut()->Error("MainWindow::RenderWindowActive","unknown rendermode from %s", sender->GetDatasetName().toStdString().c_str()); break;
		}
	}
}

void MainWindow::RenderWindowClosing(RenderWindow* sender) {
	m_MasterController.DebugOut()->Message("MainWindow::RenderWindowClosing","ACK that %s is now closing", sender->GetDatasetName().toStdString().c_str());

	QList<QListWidgetItem*> l = listWidget_Lock->findItems(sender->GetWindowID(),  Qt::MatchExactly);
	assert(l.size() == 1); // if l.size() != 1 something went wrong during the creation of the list
	delete l[0];

	m_ActiveRenderWin = NULL;
	disconnect(sender, SIGNAL(WindowActive(RenderWindow*)), this, SLOT(RenderWindowActive(RenderWindow*)));
	disconnect(sender, SIGNAL(WindowClosing(RenderWindow*)), this, SLOT(RenderWindowClosing(RenderWindow*)));

	m_1DTransferFunction->SetData(NULL, NULL);
	m_1DTransferFunction->update();
	m_2DTransferFunction->SetData(NULL, NULL);
	m_2DTransferFunction->update();
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

void MainWindow::CheckForRedraw() {
	for (int i = 0;i<mdiArea->subWindowList().size();i++) {
		QWidget* w = mdiArea->subWindowList().at(i)->widget();			
		qobject_cast<RenderWindow*>(w)->CheckForRedraw();
	}
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

	unsigned int iPaintmode = Q1DT_PAINT_NONE;
	

	if (checkBox_Red->isChecked() ) iPaintmode |= Q1DT_PAINT_RED;
	if (checkBox_Green->isChecked() ) iPaintmode |= Q1DT_PAINT_GREEN;
	if (checkBox_Blue->isChecked() ) iPaintmode |= Q1DT_PAINT_BLUE;
	if (checkBox_Alpha->isChecked() ) iPaintmode |= Q1DT_PAINT_ALPHA;

	m_1DTransferFunction->SetPaintmode(iPaintmode);
}

void MainWindow::Transfer1DRadioClicked() {
	// determine current CB state
	unsigned int iRadioState;
	if (radioButton_User->isChecked()) iRadioState = 0;
		else if (radioButton_Luminance->isChecked()) iRadioState = 1;
			else iRadioState = 2;

	// if we are in usermode do nothing
	if (iRadioState == 0) return;

	// apply iRadioState
	checkBox_Red->setChecked(true);	
	checkBox_Green->setChecked(true);	
	checkBox_Blue->setChecked(true);	
	checkBox_Alpha->setChecked(iRadioState==2);

	m_1DTransferFunction->SetPaintmode(Q1DT_PAINT_RED | Q1DT_PAINT_GREEN | Q1DT_PAINT_BLUE | ((iRadioState==2) ? Q1DT_PAINT_ALPHA : Q1DT_PAINT_NONE));
}

void MainWindow::Use1DTrans() {
	checkBox_Use2DTrans->setChecked(false);
	checkBox_Use2DTrans->setEnabled(true);
	checkBox_UseIso->setChecked(false);
	checkBox_UseIso->setEnabled(true);
	checkBox_Use1DTrans->setEnabled(false);
	checkBox_Use1DTrans->setChecked(true);
	radioButton_1DTrans->setChecked(true);

	m_1DTransferFunction->setEnabled(true);
	m_2DTransferFunction->setEnabled(false);
	// todo disable iso controlls

	if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(RM_1DTRANS);
}

void MainWindow::Use2DTrans() {
	checkBox_Use1DTrans->setChecked(false);
	checkBox_Use1DTrans->setEnabled(true);
	checkBox_UseIso->setChecked(false);
	checkBox_UseIso->setEnabled(true);
	checkBox_Use2DTrans->setEnabled(false);
	checkBox_Use2DTrans->setChecked(true);
	radioButton_2DTrans->setChecked(true);

	m_1DTransferFunction->setEnabled(false);
	m_2DTransferFunction->setEnabled(true);

	// todo disable iso controlls

	if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(RM_2DTRANS);
}

void MainWindow::UseIso() {
	checkBox_Use2DTrans->setChecked(false);
	checkBox_Use2DTrans->setEnabled(true);
	checkBox_Use1DTrans->setChecked(false);
	checkBox_Use1DTrans->setEnabled(true);
	checkBox_UseIso->setEnabled(false);
	checkBox_UseIso->setChecked(true);
	radioButton_Iso->setChecked(true);

	m_1DTransferFunction->setEnabled(false);
	m_2DTransferFunction->setEnabled(false);
	// todo enable iso controlls
	if (m_ActiveRenderWin) m_ActiveRenderWin->SetRendermode(RM_ISOSURFACE);
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

    m_1DTransferFunction = new Q1DTransferFunction(m_MasterController, dockWidgetContents_6);
	horizontalLayout_13->addWidget(m_1DTransferFunction);
	m_2DTransferFunction = new Q2DTransferFunction(m_MasterController, frame_5);
	verticalLayout_11->addWidget(m_2DTransferFunction);

	Use2DTrans();

	connect(m_2DTransferFunction, SIGNAL(SwatchChange()), this, SLOT(SwatchesChanged()));
	connect(listWidget_Swatches, SIGNAL(currentRowChanged(int)), m_2DTransferFunction, SLOT(SetActiveSwatch(int)));
	connect(listWidget_Swatches, SIGNAL(currentRowChanged(int)), this, SLOT(UpdateSwatchButtons()));
	connect(listWidget_Gradient, SIGNAL(currentRowChanged(int)), this, SLOT(UpdateGradientButtons()));	

	connect(pushButton_AddPoly,  SIGNAL(clicked()), m_2DTransferFunction, SLOT(AddSwatch()));
	connect(pushButton_DelPoly,  SIGNAL(clicked()), m_2DTransferFunction, SLOT(DeleteSwatch()));
	connect(pushButton_UpPoly,   SIGNAL(clicked()), m_2DTransferFunction, SLOT(UpSwatch()));
	connect(pushButton_DownPoly, SIGNAL(clicked()), m_2DTransferFunction, SLOT(DownSwatch()));

	for (unsigned int i = 0; i < ms_iMaxRecentFiles; ++i) {
		m_recentFileActs[i] = new QAction(this);
		m_recentFileActs[i]->setVisible(false);
		connect(m_recentFileActs[i], SIGNAL(triggered()),this, SLOT(OpenRecentFile()));
		menuLast_Used_Projects->addAction(m_recentFileActs[i]);
	}

	// this widget is used to share the contexts amongst the render windows
	m_glShareWidget = new QGLWidget(this);
	this->horizontalLayout->addWidget(m_glShareWidget);

	LoadDataset("DEBUG");
}

void MainWindow::OpenRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) LoadDataset(action->data().toString());
}

void MainWindow::ClearDebugWin() {
	listWidget_3->clear();
}

void MainWindow::SetDebugViewMask() {
	m_DebugOut->m_bShowErrors = checkBox_ShowDebugErrors->isChecked();
	m_DebugOut->m_bShowWarnings = checkBox_ShowDebugWarnings->isChecked();
	m_DebugOut->m_bShowMessages = checkBox_ShowDebugMessages->isChecked();
	m_DebugOut->m_bShowOther = checkBox_ShowDebugOther->isChecked();
}