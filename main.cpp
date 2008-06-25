#include <QtGui/QApplication>
#include "ImageVis3D.h"

int main(int argc, char* argv[])
{
	QApplication app( argc, argv );
	MainWindow mainWindow(0, Qt::Window);
	mainWindow.show();
	return app.exec();
}
