#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <tclap/CmdLine.h>

#include "DataSource.h"
#include "BlockInfo.h"

using namespace boost;
using namespace std;
using namespace tuvok;

#ifdef _WIN32
  // CRT's memory leak detection
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif

int main(int argc, char* argv[])
{
  HRConsoleOut* debugOut = new HRConsoleOut();
  debugOut->SetOutput(true, true, true, false);
  debugOut->SetClearOldMessage(true);

  Controller::Instance().AddDebugOut(debugOut);
  MESSAGE(" "); // get rid of "connected to this debug out" message
  cout << endl;

  #ifdef _WIN32
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
      _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif
  #endif

  string strUVFName = "";

  uint32_t iSizeX = 100;
  uint32_t iSizeY = 200;
  uint32_t iSizeZ = 300;
  uint32_t iBitSize = 8;
  uint32_t iBrickSize = DEFAULT_BRICKSIZE;
  bool bCreateFile;
  bool bZlib;
  bool bVerify;
  bool bShow1dhist;
  bool bShow2dhist;
  bool bMandelbulb;
  bool bShowData;
  bool bUseToCBlock;
  bool bKeepRaw;

  try {
    TCLAP::CmdLine cmd("UVF diagnostic tool");
    TCLAP::MultiArg<std::string> inputs("f", "file", "input/output file.",
                                        true, "filename");
    TCLAP::SwitchArg noverify("n", "noverify", "disable the checksum test",
                              false);
    TCLAP::SwitchArg hist1d("1", "1dhist", "output the 1D histogram", false);
    TCLAP::SwitchArg hist2d("2", "2dhist", "output the 2D histogram", false);
    TCLAP::SwitchArg create("c", "create", "create instead of read a UVF",
                            false);
    TCLAP::SwitchArg zlib("o", "compress", "create a zlib compressed UVF",
                            false);
    TCLAP::SwitchArg mandelbulb("m", "mandelbulb", "compute mandelbulb "
                                     "fractal instead of simple sphere", false);
    TCLAP::SwitchArg output_data("d", "data", "display data at finest"
                                 " resolution", false);
    std::string uint = "unsigned integer";
    TCLAP::ValueArg<size_t> sizeX("x", "sizeX", "width of created volume",
                                  false, static_cast<size_t>(100), uint);
    TCLAP::ValueArg<size_t> sizeY("y", "sizeY", "height of created volume",
                                  false, static_cast<size_t>(200), uint);
    TCLAP::ValueArg<size_t> sizeZ("z", "sizeZ", "depth of created volume",
                                  false, static_cast<size_t>(300), uint);
    TCLAP::ValueArg<size_t> bits("b", "bits", "bit width of created volume",
                                 false, static_cast<size_t>(8), uint);
    TCLAP::ValueArg<size_t> bsize("s", "bricksize", "maximum width, "
                                  "in any dimension, for a created volume",
                                  false, static_cast<size_t>(256), uint);
    TCLAP::SwitchArg use_rdb("r", "rdb", "use older raster data block", false);
    TCLAP::SwitchArg keep_raw("k", "keep", "keep intermediate raw file "
                                          "during test data generation", false);

    cmd.add(inputs);
    cmd.add(noverify);
    cmd.add(hist1d);
    cmd.add(hist2d);
    cmd.add(create);
    cmd.add(zlib);
    cmd.add(mandelbulb);
    cmd.add(keep_raw);
    cmd.add(sizeX);
    cmd.add(sizeY);
    cmd.add(sizeZ);
    cmd.add(bits);
    cmd.add(bsize);
    cmd.add(use_rdb);
    cmd.add(output_data);
    cmd.parse(argc, argv);

    /// @todo FIXME support a list of filenames and process them in sequence.
    strUVFName = inputs.getValue()[0];
    iSizeX = static_cast<uint32_t>(sizeX.getValue());
    iSizeY = static_cast<uint32_t>(sizeY.getValue());
    iSizeZ = static_cast<uint32_t>(sizeZ.getValue());
    iBitSize = static_cast<uint32_t>(bits.getValue());
    iBrickSize = static_cast<uint32_t>(bsize.getValue());

    bCreateFile = create.getValue();
    bZlib = zlib.getValue();
    bVerify = !noverify.getValue();
    bShow1dhist = hist1d.getValue();
    bShow2dhist = hist2d.getValue();
    bMandelbulb = mandelbulb.getValue();
    bShowData = output_data.getValue();
    bUseToCBlock = !use_rdb.getValue();
    bKeepRaw = keep_raw.getValue();
  } catch(const TCLAP::ArgException& e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << "\n";
    return EXIT_FAILURE;
  }

  UINT64VECTOR3 vSize(iSizeX, iSizeY, iSizeZ);

  if (strUVFName.empty()) {
    cerr << endl << "Missing Argument -f or filename was empty" << endl;
    return EXIT_FAILURE;
  }

  if (iBitSize != 8 && iBitSize != 16) {
    cerr << endl << "Argument -bits can only be 8 or 16" << endl;
    return EXIT_FAILURE;
  }

  if (bZlib && !bUseToCBlock) {
    cerr << endl << "Brick compression is not available with the "
                    "old file format (-r switch)" << endl;
    return EXIT_FAILURE;
  }

  if (bCreateFile) {
    if (!CreateUVFFile(strUVFName, vSize, iBitSize, bMandelbulb, iBrickSize,
                       bUseToCBlock, bKeepRaw, bZlib))
      return EXIT_FAILURE;
  } else {
    if (!DisplayUVFInfo(strUVFName, bVerify, bShowData, bShow1dhist, 
                        bShow2dhist))
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 
   2008 Scientific Computing and Imaging Institute,
   University of Utah.
   2012 Interactive Visualization and Data Analysis Group

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
