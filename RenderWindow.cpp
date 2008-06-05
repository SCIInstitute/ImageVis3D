#include "RenderWindow.h"

RenderWindow::RenderWindow(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);
	LoadMockupImage();
}

RenderWindow::~RenderWindow() {

}

void RenderWindow::LoadMockupImage()
{
	QGraphicsScene * scene = new QGraphicsScene;
	QPixmap lPixmap;

	lPixmap.load("RenderWin1.png");
	scene->addPixmap(lPixmap);
	graphicsView->setScene(scene);
}
