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

/// \author James Hughes
/// \date   November 2012

#include <string>
#include <fstream>
#include <iostream>
#include <streambuf>

#include "TuvokLuaScriptExec.h"
#include "Controller/Controller.h"
#include "Controller/MasterController.h"

namespace tuvok {

//------------------------------------------------------------------------------
TuvokLuaScriptExec::TuvokLuaScriptExec()
{
  // Register context creation function.
}


//------------------------------------------------------------------------------
TuvokLuaScriptExec::~TuvokLuaScriptExec()
{
}

//------------------------------------------------------------------------------
void TuvokLuaScriptExec::execFile(const std::string& filename)
{
  std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();

  // Execute entire file in Lua.
  try
  {
    lua_State* L = ss->getLuaState();
    luaL_loadfile(L, filename.c_str());
    const int err = lua_pcall(L, 0, LUA_MULTRET, 0);
    switch(err) {
      case 0: /* success. */ break;
      case LUA_ERRRUN:
        std::cerr << "Error interpreting file: " << filename << std::endl;
        if (lua_gettop(L) != 0 && lua_isstring(L, lua_gettop(L)))
          std::cerr << "Error: " << lua_tostring(L, lua_gettop(L)) << std::endl;
        break;
      case LUA_ERRMEM:
        std::cerr << "Memory allocation error.\n";
        break;
      case LUA_ERRERR:
        std::cerr << "Error running Lua error function(?!)\n";
        break;
      default:
        std::cerr << "Unknown Lua error (" << err << ").\n";
        break;
    }
  }
  catch (...)
  {
    // If an exception occurred in Lua, the description will be fairly verbose.
    throw;
  }
}


} // end of namespace tuvok
