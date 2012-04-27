/*
 For more information, please see: http://software.sci.utah.edu

 The MIT License

 Copyright (c) 2012 Scientific Computing and Imaging Institute,
 University of Utah.


 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

/**
 \brief Combination of the debug window and a new scripting window.
 */


#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QListWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QLineEdit>

#include "DebugScriptWindow.h"

//-----------------------------------------------------------------------------
DebugScriptWindow::DebugScriptWindow(QWidget* parent)
: QDockWidget("Debug / Scripting Window", parent)
{
  // TODO: Implement as setupUI function (modeling after QT).

  mDockWidgetContents = new QWidget();
  mDockWidgetContents->setObjectName(QString::fromUtf8("DebugWindowContents"));
  this->setWidget(mDockWidgetContents);

  mMainLayout = new QVBoxLayout(mDockWidgetContents);
  mMainLayout->setSpacing(6);
  mMainLayout->setContentsMargins(9, 9, 9, 9);
  mMainLayout->setObjectName(QString::fromUtf8("verticalLayout"));

  mTabWidget = new QTabWidget;
  mMainLayout->addWidget(mTabWidget);

  {
	  QWidget* scriptTabContents = new QWidget();
	  mTabWidget->addTab(scriptTabContents, QString::fromUtf8("script"));

	  QHBoxLayout* scriptLayout = new QHBoxLayout(scriptTabContents);
	  scriptTabContents->setLayout(scriptLayout);

	  {
		  QWidget* outputContents = new QWidget();
		  scriptLayout->addWidget(outputContents);

		  QVBoxLayout* outputLayout = new QVBoxLayout();
		  outputContents->setLayout(outputLayout);

		  QListWidget* scriptOutput = new QListWidget();
		  outputLayout->addWidget(scriptOutput);

		  QLineEdit* textLine = new QLineEdit();
		  outputLayout->addWidget(textLine);
	  }

	  {
		  QWidget* editContents = new QWidget();
		  scriptLayout->addWidget(editContents);

		  QTextEdit* text = new QTextEdit();
		  scriptLayout->addWidget(text);
	  }
  }

  QWidget* debugTabContents = new QWidget();
  mTabWidget->addTab(debugTabContents, QString::fromUtf8("debug"));

  hookLuaFunctions();
}

//-----------------------------------------------------------------------------
DebugScriptWindow::~DebugScriptWindow()
{

}

//-----------------------------------------------------------------------------
void DebugScriptWindow::hookLuaFunctions()
{

}

//-----------------------------------------------------------------------------
void DebugScriptWindow::hook_logInfo(std::string info)
{
	(void)info;
}

//-----------------------------------------------------------------------------
void DebugScriptWindow::hook_logError(std::string error)
{
	(void)error;
}

