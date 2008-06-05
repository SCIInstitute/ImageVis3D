#ifndef TRANSDIALOG2D_H
#define TRANSDIALOG2D_H

#include "ui_TransDialog2D.h"


class TransDialog2D : public QDialog, protected Ui_TransDialog2D
{
	Q_OBJECT
	public:
		TransDialog2D(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~TransDialog2D();

	private :
		void LoadMockupImage();

};

#endif // TRANSDIALOG2D_H
