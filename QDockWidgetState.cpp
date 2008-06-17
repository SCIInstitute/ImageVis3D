#include "QDockWidgetState.h"

QDockWidgetState::QDockWidgetState(QDockWidget* widget, QAction* action, Qt::DockWidgetArea defaultDockWidgetArea) :
	m_widget(widget), 
	m_action(action), 
	m_defaultDockWidgetArea(defaultDockWidgetArea)
{
	// TODO
}

QDockWidgetState::~QDockWidgetState(void)
{
}

void QDockWidgetState::Apply() {
	// TODO
}

void QDockWidgetState::Get() {
	// TODO
}