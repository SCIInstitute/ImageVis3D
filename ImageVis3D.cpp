#include "ImageVis3D.h"

#include <QTGui/QMdiSubWindow>
#include <QTGui/QFileDialog>

MainWindow::MainWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QMainWindow(parent, flags)
{
	setupUi(this);
}

MainWindow::~MainWindow()
{

}

void MainWindow::Show1DTransferFunction() {
	m_TransDialog1D.show();
}

void MainWindow::Show2DTransferFunction() {
	m_TransDialog2D.show();
}

void MainWindow::CloneCurrentView() {
	 RenderWindow *renderWin = CreateNewRenderWindow();
	 renderWin->show();
}

void MainWindow::LoadDataset() {
     QString fileName = QFileDialog::getOpenFileName(this);
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