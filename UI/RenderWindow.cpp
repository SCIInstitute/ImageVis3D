#include "RenderWindow.h"

#include <QtGui/QtGui>
#include <QtOpenGL/QtOpenGL>
#include <math.h>
#include <Basics/SysTools.h>
#include <assert.h>

RenderWindow::RenderWindow(QString dataset, QListWidget *listWidget_Lock, unsigned int iCounter, QGLWidget* glShareWidget, QWidget* parent, Qt::WindowFlags flags) :
	QGLWidget(parent, glShareWidget,  flags),
	m_strDataset(dataset),
	m_listWidget_Lock(listWidget_Lock),
	m_iCurrentView(0)
{	

	m_strID = tr("[%1] %2").arg(iCounter).arg(m_strDataset);
	setWindowTitle(m_strID);
	m_listWidget_Lock->addItem(m_strID);

	ToggleRenderWindowView1x3();

	m_IDTex[0] = 0;
	m_IDTex[1] = 0;
	m_IDTex[2] = 0;
	xRot = 0;
}





RenderWindow::~RenderWindow()
{
//	makeCurrent();
//	glDeleteLists(object, 1);
}

QSize RenderWindow::minimumSizeHint() const
{
	return QSize(50, 50);
}

QSize RenderWindow::sizeHint() const
{
	return QSize(400, 400);
}

void RenderWindow::initializeGL()
{
	QColor trolltechPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
	qglClearColor(trolltechPurple.dark());
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	
	m_IDTex[0] = bindTexture(QPixmap(SysTools::GetFromResourceOnMac("RenderWin1x3.png").c_str()),GL_TEXTURE_2D);
	m_IDTex[1] = bindTexture(QPixmap(SysTools::GetFromResourceOnMac("RenderWin2x2.png").c_str()),GL_TEXTURE_2D);
	m_IDTex[2] = bindTexture(QPixmap(SysTools::GetFromResourceOnMac("RenderWin1.png").c_str()),GL_TEXTURE_2D);
}

void RenderWindow::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslated(0.0, 0.0, -10.0);
	glRotated(xRot / 16.0, 1.0, 0.0, 0.0);

	glBindTexture(GL_TEXTURE_2D, m_IDTex[m_iCurrentView]);

	glBegin(GL_QUADS);
		glColor4d(1,1,1,1);
		glTexCoord2d(0,0);
		glVertex3d(-0.5,  0.5, -0.05);
		glTexCoord2d(1,0);
		glVertex3d( 0.5,  0.5, -0.05);
		glTexCoord2d(1,1);
		glVertex3d( 0.5, -0.5, -0.05);
		glTexCoord2d(0,1);
		glVertex3d(-0.5, -0.5, -0.05);
	glEnd();
}

void RenderWindow::resizeGL(int width, int height)
{
	int side = qMin(width, height);
	glViewport((width - side) / 2, (height - side) / 2, side, side);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
	glMatrixMode(GL_MODELVIEW);
}

void RenderWindow::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();

	if (event->buttons() & Qt::RightButton) {
		m_iCurrentView = (m_iCurrentView + 1 ) %3;
		emit RenderWindowViewChanged(m_iCurrentView);
		updateGL();
	}
}

void RenderWindow::mouseMoveEvent(QMouseEvent *event)
{
	// int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton) {
		int angle = xRot + 8 * dy;
		normalizeAngle(&angle);
		if (angle != xRot) {
			xRot = angle;
			updateGL();
		}
	}
	lastPos = event->pos();
}

void RenderWindow::normalizeAngle(int *angle)
{
	while (*angle < 0) *angle += 360 * 16;
	while (*angle > 360 * 16) *angle -= 360 * 16;
}

void RenderWindow::ToggleRenderWindowView1x3() {
	m_iCurrentView = 0;
	emit RenderWindowViewChanged(m_iCurrentView);
	updateGL();
}

void RenderWindow::ToggleRenderWindowView2x2() {
	m_iCurrentView = 1;
	emit RenderWindowViewChanged(m_iCurrentView);
	updateGL();
}

void RenderWindow::ToggleRenderWindowViewSingle() {
	m_iCurrentView = 2;
	emit RenderWindowViewChanged(m_iCurrentView);
	updateGL();
}

void RenderWindow::closeEvent(QCloseEvent *event) {
	QGLWidget::closeEvent(event);

	QList<QListWidgetItem*> l = m_listWidget_Lock->findItems(m_strID,  Qt::MatchExactly);
	assert(l.size() == 1); // if l.size() != 1 something went wrong during the creation of the list
	delete l[0];
}
