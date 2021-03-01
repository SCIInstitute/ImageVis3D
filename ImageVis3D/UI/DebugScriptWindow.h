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

#ifndef DEBUGSCRIPTWINDOW_H_
#define DEBUGSCRIPTWINDOW_H_

#include <QtWidgets/QDockWidget>
#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/LuaScripting/LuaMemberReg.h"

class QVBoxLayout;
class QTabWidget;
class QPushButton;
class QComboBox;
class QTextEdit;
class QLineEdit;
class QListWidget;

class DebugScriptWindow: public QDockWidget
{
  Q_OBJECT
public:

  DebugScriptWindow(tuvok::MasterController& controller, QWidget* parent);
  virtual ~DebugScriptWindow();

protected slots:

  void execClicked();
  void oneLineEditOnReturnPressed();
  void oneLineEditOnEdited(const QString&);
  void exampComboIndexChanged(int index);
  void fixFont();

protected:

  bool eventFilter(QObject *obj, QEvent *event);

private:

  void setupUI();
  void hookLuaFunctions();
  void execLua(const std::string& cmd);

  // Lua registered function hooks.
  void hook_logInfo(std::string info);
  void hook_logWarning(std::string warn);
  void hook_logError(std::string error);

  QVBoxLayout*  mMainLayout;
  QComboBox*    mScriptExamplesBox;
  QPushButton*  mExecButton;
  QListWidget*  mListWidget;

  QTextEdit*    mScriptTextEdit;
  QLineEdit*    mScriptOneLineEdit;

  std::vector<std::string>  mSavedInput;
  int                       mSavedInputPos;

  tuvok::MasterController&               mController;
  tuvok::LuaMemberReg                    mMemReg;
  std::shared_ptr<tuvok::LuaScripting>   mLua;

  bool                                  mInFixFont;


};
#endif /* DEBUGSCRIPTWINDOW_H_ */
