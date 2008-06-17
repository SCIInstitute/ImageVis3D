#pragma once

#ifndef QDOCKWIDGETSTATE_H
#define QDOCKWIDGETSTATE_H

#include <QtGui/QAction>
#include <QtGui/QDockWidget>

class QDockWidgetState
{
public:
	QDockWidgetState(QDockWidget* widget, QAction* action, Qt::DockWidgetArea defaultDockWidgetArea);
	~QDockWidgetState(void);

	void Apply();
	void Get();

private:
	QDockWidget* m_widget;
	QAction* m_action;
	Qt::DockWidgetArea m_defaultDockWidgetArea;

	Qt::DockWidgetArea m_DockWidgetArea;


};

QT_END_NAMESPACE

#endif // QDOCKWIDGETSTATE_H

