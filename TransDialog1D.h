#ifndef TRANSDIALOG1D_H
#define TRANSDIALOG1D_H

#include "ui_TransDialog1D.h"


class TransDialog1D : public QDialog, protected Ui_TransDialog1D
{
	Q_OBJECT
	public:
		TransDialog1D(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~TransDialog1D();

	protected slots :
		void SetUserMode();
		void SetIntensityMode();
		void SetLuminanceMode();
};

#endif // TRANSDIALOG1D_H
