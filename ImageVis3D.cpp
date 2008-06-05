#include "ImageVis3D.h"

MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags)
{
	setupUi(this);
	HideAllTools();
	HideAllFilters();
	HideVisualizationWindows();
}

MainWindow::~MainWindow()
{

}

void MainWindow::HideAllTools()
{
	polyline_tool_widget->hide();
	paintbrush_tool_widget->hide();
	resample_tool_widget->hide();
	crop_tool_widget->hide();
}

void MainWindow::HideAllFilters()
{
	Gaussian_filter_widget->hide();
	Median_filter_widget->hide();
	Histogram_filter_widget->hide();
}

void MainWindow::HideVisualizationWindows()
{
/*	widget_2By2->hide();
	widget_3By1->hide();
	widget_Single->hide();*/
}


void MainWindow::Show1DTransferFunction() {
	m_TransDialog1D.show();
}

void MainWindow::Show2DTransferFunction() {
	m_TransDialog2D.show();
}


void MainWindow::AddNewRenderWindow() {
     RenderWindow *renderWin = CreateNewRenderWindow();
     renderWin->show();
}

 RenderWindow *MainWindow::CreateNewRenderWindow()
 {
     RenderWindow *renderWin = new RenderWindow;
     mdiArea->addSubWindow(renderWin);

//     connect(child, SIGNAL(copyAvailable(bool)),  cutAct, SLOT(setEnabled(bool)));
//     connect(child, SIGNAL(copyAvailable(bool)),  copyAct, SLOT(setEnabled(bool)));

     return renderWin;
 }