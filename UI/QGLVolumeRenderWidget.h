#pragma once

#ifndef QGLVOLUMERENDERWIDGET_H
#define QGLVOLUMERENDERWIDGET_H

#include <QtOpenGL/QGLWidget>

class QGLVolumeRenderWidget : public QGLWidget
{
	Q_OBJECT

	public:
		QGLVolumeRenderWidget(QWidget *parent = 0);
		virtual ~QGLVolumeRenderWidget(void);

		QSize minimumSizeHint() const;
		QSize sizeHint() const;
	public slots:
		void setXRotation(int angle);
		void setYRotation(int angle);
		void setZRotation(int angle);

	signals:
		void xRotationChanged(int angle);
		void yRotationChanged(int angle);
		void zRotationChanged(int angle);

	protected:
		void initializeGL();
		void paintGL();
		void resizeGL(int width, int height);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

	private:
		GLuint makeObject();
		void quad(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2, GLdouble x3, GLdouble y3, GLdouble x4, GLdouble y4);
		void extrude(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
		void normalizeAngle(int *angle);

	GLuint object;
		int xRot;
		int yRot;
		int zRot;
		QPoint lastPos;
		QColor trolltechGreen;
		QColor trolltechPurple;
};

#endif // QGLVOLUMERENDERWIDGET_H
