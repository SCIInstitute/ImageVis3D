/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
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

/// Simple batch renderer using Tuvok.
/// This file is dead simple as most of the logic resides in Lua files.

#include <cstdlib>
#include <iostream>

#include "GLEW/GL/glew.h"
#include "tclap/CmdLine.h"

#include "LuaScripting/LuaScripting.h"
#include "LuaScripting/TuvokSpecific/LuaTuvokTypes.h"
#include "Controller/Controller.h"
#include "Controller/MasterController.h"

#include "BatchContext.h"
#include "TuvokLuaScriptExec.h"

using namespace std;
using namespace tuvok;

std::shared_ptr<BatchContext> createContext(uint32_t width, uint32_t height,
                                            int32_t color_bits,
                                            int32_t depth_bits,
                                            int32_t stencil_bits,
                                            bool double_buffer, bool visible)
{
  std::shared_ptr<BatchContext> ctx(
      BatchContext::Create(width,height, color_bits,depth_bits,stencil_bits, 
                           double_buffer,visible));
  if(!ctx->isValid() || ctx->makeCurrent() == false) 
  {
    std::cerr << "Could not utilize context.";
    return std::shared_ptr<BatchContext>();
  }

  return ctx;
}

int main(int argc, const char* argv[])
{
  // Read Lua filename from the first program argument
  std::string filename;
  bool debug = false;
  try
  {
    TCLAP::CmdLine cmd("Lua batch renderer");
    TCLAP::ValueArg<string> luaFile("f", "script", "Script to execute.", true,
                                    "", "filename");
    TCLAP::SwitchArg dbg("g", "debug", "Enable debugging mode", false);
    cmd.add(luaFile);
    cmd.add(dbg);
    cmd.parse(argc, argv);

    filename = luaFile.getValue();
    debug = dbg.getValue();
  }
  catch (const TCLAP::ArgException& e)
  {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
    return EXIT_FAILURE;
  }
  if(debug) {
    Controller::Instance().DebugOut()->SetOutput(true, true, true, true);
  } else {
    Controller::Instance().DebugOut()->SetOutput(true, true, false, false);
  }

  try
  {
    // Register context creation function
    std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
    /// \todo Investigate why we can't use lambdas in function regisrtation.
    ss->registerFunction(createContext, "tuvok.createContext",
                         "Creates a rendering context and returns it.", false);

    TuvokLuaScriptExec luaExec;
    luaExec.execFile(filename);
  } 
  catch(const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
}

