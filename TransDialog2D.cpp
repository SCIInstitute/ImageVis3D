#include "TransDialog2D.h"

TransDialog2D::TransDialog2D(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);
	LoadMockupImage();
}

TransDialog2D::~TransDialog2D() {

}

void TransDialog2D::LoadMockupImage() {
	// TODO
}