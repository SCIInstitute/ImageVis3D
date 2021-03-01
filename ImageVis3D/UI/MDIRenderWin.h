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
 \brief Subclasses QMdiSubWindow so that we have more complete control over
        the destruction of a render window. Necessary so that undo functions
        properly when the window is destroyed.
 */

#ifndef MDIRENDERWIN_H_
#define MDIRENDERWIN_H_

#include <string>
#include <StdDefines.h>

#include <QtWidgets/QMdiSubWindow>

#include "../Tuvok/Controller/MasterController.h"
#include "../Tuvok/LuaScripting/LuaScripting.h"
#include "../Tuvok/LuaScripting/LuaClassRegistration.h"

class RenderWindow;

class MDIRenderWin : public QMdiSubWindow
{
public:
  MDIRenderWin(tuvok::MasterController& masterController,
               RenderWindow* renderWin,
               QWidget *parent = 0,
               Qt::WindowFlags flags = 0);

protected:

  /// Used to explicitly kill off our child render window.
  void closeEvent(QCloseEvent *closeEvent);

private:

  tuvok::MasterController&  mMasterController;
  RenderWindow*             mRenderWindow;
};

#endif /* MDIRENDERWIN_H_ */
