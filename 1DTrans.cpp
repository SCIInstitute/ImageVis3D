#include "1DTrans.h"

TransDialog1D::TransDialog1D(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);
	LoadMockupImage();
}

TransDialog1D::~TransDialog1D() {

}

void TransDialog1D::LoadMockupImage()
{
	QGraphicsScene * scene = new QGraphicsScene;
	QPixmap lPixmap;

	lPixmap.load("1DTrans.png");
	scene->addPixmap(lPixmap);
	graphicsView->setScene(scene);
}

