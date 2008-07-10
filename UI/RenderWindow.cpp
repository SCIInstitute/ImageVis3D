#include "RenderWindow.h"
#include <Basics/SysTools.h>
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

void RenderWindow::LoadImages() {
	m_Pixmap1x3.load(SysTools::GetFromResourceOnMac("RenderWin1x3.png").c_str());
	m_Pixmap2x2.load(SysTools::GetFromResourceOnMac("RenderWin2x2.png").c_str());
	m_PixmapSingle.load(SysTools::GetFromResourceOnMac("RenderWin1.png").c_str());
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
