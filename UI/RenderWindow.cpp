#include "RenderWindow.h"
#include <assert.h>


RenderWindow::RenderWindow(QString dataset, QListWidget *listWidget_Lock, unsigned int iCounter, QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) :
	QDialog(parent, flags),
	m_strDataset(dataset),
	m_listWidget_Lock(listWidget_Lock)
{	
	setupUi(this);

	m_strID = tr("[%1] %2").arg(iCounter).arg(m_strDataset);
	setWindowTitle(m_strID);
	m_listWidget_Lock->addItem(m_strID);

	LoadImages();

	ToggleRenderWindowView1x3();
}

RenderWindow::~RenderWindow() {

}


#include <CoreFoundation/CoreFoundation.h>

void RenderWindow::LoadImages() {
#if defined(Q_OS_MACX) 
	CFURLRef    imageURL = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR("RenderWin1x3"), CFSTR("png"), NULL );
	CFStringRef macPath = CFURLCopyFileSystemPath(imageURL, kCFURLPOSIXPathStyle);
	const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
	m_Pixmap1x3.load(pathPtr);
	
	
	m_Pixmap2x2.load(pathPtr);
	m_PixmapSingle.load(pathPtr);
#else
	m_Pixmap1x3.load("../Resources/RenderWin1x3.png");
	m_Pixmap2x2.load("Resources/RenderWin2x2.png");
	m_PixmapSingle.load("RenderWin1.png");
#endif
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

void RenderWindow::closeEvent(QCloseEvent *event) {
	QDialog::closeEvent(event);

	QList<QListWidgetItem*> l = m_listWidget_Lock->findItems(m_strID,  Qt::MatchExactly);

	assert(l.size() == 1); // if l.size() != 1 something went wrong during the creation of the list

	delete l[0];
}
