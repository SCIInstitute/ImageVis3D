#include "ImageVis3D.h"


MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags)
{
	setupUi(this);
	hide_tools();
	hide_filters();
}

MainWindow::~MainWindow()
{

}

void MainWindow::hide_tools()
{
	polyline_tool_widget->hide();
	paintbrush_tool_widget->hide();
	resample_tool_widget->hide();
	crop_tool_widget->hide();
}

void MainWindow::hide_filters()
{
	Gaussian_filter_widget->hide();
	Median_filter_widget->hide();
	Histogram_filter_widget->hide();
}


void MainWindow::Show1DTransferFunction() {
	m_TransDialog1D.show();
}