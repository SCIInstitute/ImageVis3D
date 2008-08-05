#include "RenderWindow.h"

#include <QtGui/QtGui>
#include <QtOpenGL/QtOpenGL>
#include <assert.h>

RenderWindow::RenderWindow(MasterController& masterController, QString dataset, QListWidget *listWidget_Lock, unsigned int iCounter, QGLWidget* glShareWidget, QWidget* parent, Qt::WindowFlags flags) :
	m_Renderer((GPUSBVR*)masterController.RequestNewVolumerenderer(OPENGL_SBVR)),
	QGLWidget(parent, glShareWidget,  flags),
	m_MasterController(masterController),
	m_strDataset(dataset),
	m_listWidget_Lock(listWidget_Lock)
{	

	m_strID = tr("[%1] %2").arg(iCounter).arg(m_strDataset);
	setWindowTitle(m_strID);
	m_listWidget_Lock->addItem(m_strID);

	ToggleRenderWindowView1x3();

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
	if (m_Renderer != NULL) m_Renderer->Initialize();
}

void RenderWindow::paintGL()
{
	if (m_Renderer != NULL) m_Renderer->Paint();
}

void RenderWindow::resizeGL(int width, int height)
{
	if (m_Renderer != NULL) m_Renderer->Resize(width, height);
}

void RenderWindow::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();

	if (event->buttons() & Qt::RightButton) {
		if (m_Renderer != NULL) m_Renderer->SetCurrentView((m_Renderer->GetCurrentView()+1) %3);
		emit RenderWindowViewChanged(m_Renderer->GetCurrentView());
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
			if (m_Renderer != NULL) m_Renderer->SetRotation(xRot);
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
	if (m_Renderer != NULL) m_Renderer->SetCurrentView(0);
	emit RenderWindowViewChanged(0);
	updateGL();
}

void RenderWindow::ToggleRenderWindowView2x2() {
	if (m_Renderer != NULL) m_Renderer->SetCurrentView(1);
	emit RenderWindowViewChanged(1);
	updateGL();
}

void RenderWindow::ToggleRenderWindowViewSingle() {
	if (m_Renderer != NULL) m_Renderer->SetCurrentView(2);
	emit RenderWindowViewChanged(2);
	updateGL();
}

void RenderWindow::closeEvent(QCloseEvent *event) {
	QGLWidget::closeEvent(event);

	QList<QListWidgetItem*> l = m_listWidget_Lock->findItems(m_strID,  Qt::MatchExactly);
	assert(l.size() == 1); // if l.size() != 1 something went wrong during the creation of the list
	delete l[0];
}
