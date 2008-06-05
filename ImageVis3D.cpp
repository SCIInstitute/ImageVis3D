#include "ImageVis3D.h"


MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags)
{
	setupUi(this);
	HideAllTools();
	HideAllFilters();
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


void MainWindow::Show1DTransferFunction() {
	m_TransDialog1D.show();
}

void MainWindow::Show2DTransferFunction() {
	m_TransDialog2D.show();
}
