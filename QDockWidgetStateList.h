#pragma once

#ifndef QDOCKWIDGETSTATELIST_H
#define QDOCKWIDGETSTATELIST_H

#include "QDockWidgetState.h"
#include <QtCore/QVariant>
#include <vector>

class QDockWidgetStateList
{
public:
	QDockWidgetStateList(void);
	~QDockWidgetStateList(void);

	bool LoadFromFile(QString strFilename);
	bool SaveToFile(QString strFilename);

	void Apply() {for (unsigned int i = 0;i<m_states.size();i++) m_states[i].Apply();}
	void Add(QDockWidgetState state) {m_states.push_back(state);}
	void Get() {for (unsigned int i = 0;i<m_states.size();i++) m_states[i].Get();}

private:

	std::vector<QDockWidgetState> m_states;
};

#endif // QDOCKWIDGETSTATELIST_H
