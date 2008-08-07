#pragma once

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <QtGui/QListWidget>
#include <QtOpenGL/QGLWidget>

#include "Controller/MasterController.h"

class RenderWindow : public QGLWidget
{
	Q_OBJECT
	public:
		RenderWindow(MasterController& masterController, QString dataset, unsigned int iCounter, QGLWidget* glWidget, QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~RenderWindow();

		QString GetDatasetName() {return m_strDataset;}
		QString GetWindowID() {return m_strID;}
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		AbstrRenderer* GetRenderer() {return m_Renderer;}

	public slots:
		void ToggleRenderWindowView1x3();
		void ToggleRenderWindowView2x2();
		void ToggleRenderWindowViewSingle();

	signals:
		void RenderWindowViewChanged(int iViewID);
		void WindowActive(RenderWindow* sender);
		void WindowClosing(RenderWindow* sender);

	protected:
		virtual void initializeGL();
		virtual void paintGL();
		virtual void resizeGL(int width, int height);
		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);
		virtual void closeEvent(QCloseEvent *event);
		virtual void focusInEvent(QFocusEvent * event);

	private:
		GPUSBVR*				m_Renderer;
		MasterController&		m_MasterController;
		
		void normalizeAngle(int *angle);
		int xRot;
		QPoint lastPos;
		
		QString m_strDataset;
		QString m_strID;

};

#endif // RENDERWINDOW_H
