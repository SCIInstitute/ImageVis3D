#ifndef _1DTRANS_H
#define _1DTRANS_H

#include "ui_1DTrans.h"


class TransDialog1D : public QDialog, protected Ui_TransDialog1D
{
	Q_OBJECT
	public:
		TransDialog1D(QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~TransDialog1D();

	private :
		void LoadMockupImage();

};

#endif // _1DTRANS_H
