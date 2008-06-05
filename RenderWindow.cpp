#include "RenderWindow.h"

RenderWindow::RenderWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);

	LoadImages();

	ToggleRenderWindowView1x3();
}

RenderWindow::~RenderWindow() {

}

void RenderWindow::LoadImages() {
	m_Pixmap1x3.load("RenderWin1x3.png");
	m_Pixmap2x2.load("RenderWin2x2.png");
	m_PixmapSingle.load("RenderWin1.png");
}

void RenderWindow::ToggleRenderWindowView1x3() {
	label->setPixmap(m_Pixmap1x3);
}

void RenderWindow::ToggleRenderWindowView2x2() {
	label->setPixmap(m_Pixmap2x2);
}

void RenderWindow::ToggleRenderWindowViewSingle() {
	label->setPixmap(m_PixmapSingle);
}
