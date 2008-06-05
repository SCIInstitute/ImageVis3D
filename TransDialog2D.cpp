#include "TransDialog2D.h"

TransDialog2D::TransDialog2D(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);
	LoadMockupImage();
}

TransDialog2D::~TransDialog2D() {
}

void TransDialog2D::LoadMockupImage() {
	QGraphicsScene *scene1 = new QGraphicsScene;
	QGraphicsScene *scene2 = new QGraphicsScene;
	QPixmap lPixmap1;
	QPixmap lPixmap2;

	lPixmap1.load("trans2d_1.png");
	scene1->addPixmap(lPixmap1);
	graphicsView_Histogram->setScene(scene1);

	lPixmap2.load("trans2d_2.png");
	scene2->addPixmap(lPixmap2);
	graphicsView_ColorPicker->setScene(scene2);

}