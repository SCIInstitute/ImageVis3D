#include "TransDialog1D.h"

TransDialog1D::TransDialog1D(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : QDialog(parent, flags)
{
	setupUi(this);
}

TransDialog1D::~TransDialog1D() {

}

void TransDialog1D::SetUserMode() {
	radioButton_User->setChecked(true);
}

void TransDialog1D::SetIntensityMode() {
	checkBox_Red->setChecked(true);
	checkBox_Green->setChecked(true);
	checkBox_Blue->setChecked(true);
	checkBox_Alpha->setChecked(true);
}

void TransDialog1D::SetLuminanceMode() {
	checkBox_Red->setChecked(true);
	checkBox_Green->setChecked(true);
	checkBox_Blue->setChecked(true);
	checkBox_Alpha->setChecked(false);
}