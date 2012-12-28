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

//#include "StdTuvokDefines.h"
#include <cstdlib>
#include <iostream>

#include <GL/glew.h>
#include <tclap/CmdLine.h>

#include "LuaScripting/LuaScripting.h"
#include "LuaScripting/TuvokSpecific/LuaTuvokTypes.h"

#include "context.h"

using namespace std;
using namespace tuvok;

int main(int argc, const char* argv[])
{
  // Read Lua filename from the first program argument in the 
  std::string filename;
  try
  {
    TCLAP::CmdLine cmd("Lua batch renderer");
    TCLAP::ValueArg<string> luaFile("f", "script", "Script to execute.", true,
                                    "", "filename");
    cmd.add(luaFile);
    cmd.parse(argc, argv);

    filename = luaFile.getValue();
  }
  catch (const TCLAP::ArgException& e)
  {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
    return EXIT_FAILURE;
  }

  try {
    // Create a new context.
    std::auto_ptr<TvkContext> ctx(TvkContext::Create(640,480, 32,24,8, true));
    if(!ctx->isValid() || ctx->makeCurrent() == false) {
      T_ERROR("Could not utilize context.");
      return EXIT_FAILURE;
    }
    Controller::Instance().DebugOut()->SetOutput(true,true,true,true);

    // Convert the data into a UVF.
    std::string uvf_file;
    uvf_file = SysTools::RemoveExt(filename) + ".uvf";
    const std::string tmpdir = "/tmp/";
    const bool quantize8 = false;
    Controller::Instance().IOMan()->ConvertDataset(
      filename, uvf_file, tmpdir, true, 256, 4, quantize8
    );

//    AbstrRenderer* ren;
//    ren = Controller::Instance().RequestNewVolumeRenderer(
//      MasterController::OPENGL_SBVR, false, false, false, false, false
//    );
    std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
    LuaClassInstance luaRen = ss->cexecRet<LuaClassInstance>(
        "tuvok.renderer.new",
        int(MasterController::OPENGL_SBVR), false, false, false, false, false);
    ss->cexec(luaRen.fqName() + ".loadDataset", uvf_file);
    ss->cexec(luaRen.fqName() + ".addShaderPath", "../../Shaders");
    ss->cexec(luaRen.fqName() + ".initialize", GLContext::Current(0));
    ss->cexec(luaRen.fqName() + ".resize", UINTVECTOR2(640,480));
    ss->cexec(luaRen.fqName() + ".setRendererTarget",AbstrRenderer::RT_HEADLESS);
    ss->cexec(luaRen.fqName() + ".paint");

    AbstrRenderer* ren = luaRen.getRawPointer<AbstrRenderer>(ss);
    GLRenderer* glren = dynamic_cast<GLRenderer*>(ren);
    GLFBOTex* fbo = glren->GetLastFBO();
    GLTargetBinder bind(&Controller::Instance());

    GLFrameCapture fc;
    fc.CaptureSingleFrame("test.png", fbo);

    ss->cexec(luaRen.fqName() + ".cleanup");
    Controller::Instance().ReleaseVolumeRenderer(ren);
  } catch(const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

}
