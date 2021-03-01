#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <tclap/CmdLine.h>

#include "DataSource.h"
#include "BlockInfo.h"
#include "Basics/SystemInfo.h"

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
  const uint64_t memSize = Controller::Const().SysInfo().GetCPUMemSize();
  Controller::Instance().SetMaxCPUMem(uint64_t(memSize /(1024*1024) * 0.8));
  MESSAGE(""); cout << endl;

  #ifdef _WIN32
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
      _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    #endif
  #endif

  wstring strUVFName = L"";

  uint32_t iSizeX = 100;
  uint32_t iSizeY = 200;
  uint32_t iSizeZ = 300;
  uint32_t iBitSize = 8;
  uint32_t iBrickSize = DEFAULT_BRICKSIZE;
  uint32_t iIter = 0;
  uint32_t iMem = 0;
  uint32_t iBrickLayout = 0; // 0 is default scanline layout
  uint32_t iCompression = 1; // 1 is default zlib compression
  uint32_t iCompressionLevel = 1; // generic compression level, 1 is best speed
  bool bCreateFile;
  bool bhierarchical;
  bool bVerify;
  bool bShow1dhist;
  bool bShow2dhist;
  ECreationType eCreationType;
  bool bShowData;
  bool bUseToCBlock;
  bool bKeepRaw;

  try {
    TCLAP::CmdLine cmd("UVF diagnostic and demo data generation tool");
    TCLAP::MultiArg<std::string> inputs("f", "file", "input/output file.",
                                        true, "filename");
    TCLAP::SwitchArg noverify("n", "noverify", "disable the checksum test",
                              false);
    TCLAP::SwitchArg hist1d("1", "1dhist", "output the 1D histogram", false);
    TCLAP::SwitchArg hist2d("2", "2dhist", "output the 2D histogram", false);
    TCLAP::SwitchArg create("c", "create", "create instead of read a UVF",
                            false);
    TCLAP::ValueArg<uint32_t> ctype("t", "creation-type", "What type of volume to "
                                    "create. 0: mandelbulb fractal, 1: all zeros,"
                                    "2: random values, otherwise create a sphere",
                                    false, static_cast<uint32_t>(3),
                                "volume type class");
    TCLAP::SwitchArg hierarchical("g", "hierarchical", "hierarchical generation mode", false);
    std::string uint = "unsigned integer";
    TCLAP::SwitchArg output_data("d", "data", "display data at finest"
                                 " resolution", false);
    TCLAP::ValueArg<uint32_t> iter("i", "iterations", "number of iterations "
                                   "for fractal compuation", false, 
                                   static_cast<uint32_t>(0), uint);
    TCLAP::ValueArg<uint32_t> mem("e", "memory", "gigabytes of memory "
                                   "to be used for UVF creation", false, 
                                   static_cast<uint32_t>(0), uint);
    TCLAP::ValueArg<size_t> compression("p", "compress", "UVF compression "
                                        "method 0: no compression, 1: zlib, 2: "
                                        "lzma, 3: lz4, 4: bzlib",
                                        false, static_cast<size_t>(1), uint);
    TCLAP::ValueArg<size_t> complevel("v", "level", "UVF compression level "
                                      "between (1..10)",
                                      false, static_cast<size_t>(1), uint);
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
    TCLAP::ValueArg<size_t> blayout("l", "bricklayout", "brick layout "
                                  "on disk 0: scanline, 1: morton, 2: hilbert"
                                  ", 3: random order",
                                  false, static_cast<size_t>(0), uint);
    TCLAP::SwitchArg use_rdb("r", "rdb", "use older raster data block", false);
    TCLAP::SwitchArg keep_raw("k", "keep", "keep intermediate raw file "
                                          "during test data generation", false);

    cmd.add(inputs);
    cmd.add(noverify);
    cmd.add(hist1d);
    cmd.add(hist2d);
    cmd.add(create);
    cmd.add(hierarchical);
    cmd.add(compression);
    cmd.add(complevel);
    cmd.add(ctype);
    cmd.add(mem);
    cmd.add(iter);
    cmd.add(keep_raw);
    cmd.add(sizeX);
    cmd.add(sizeY);
    cmd.add(sizeZ);
    cmd.add(bits);
    cmd.add(bsize);
    cmd.add(blayout);
    cmd.add(use_rdb);
    cmd.add(output_data);
    cmd.parse(argc, argv);

    /// @todo FIXME support a list of filenames and process them in sequence.
    strUVFName = SysTools::toWide(inputs.getValue()[0]);
    iSizeX = static_cast<uint32_t>(sizeX.getValue());
    iSizeY = static_cast<uint32_t>(sizeY.getValue());
    iSizeZ = static_cast<uint32_t>(sizeZ.getValue());
    iBitSize = static_cast<uint32_t>(bits.getValue());
    iBrickSize = static_cast<uint32_t>(bsize.getValue());
    iIter = static_cast<uint32_t>(iter.getValue());
    iMem = static_cast<uint32_t>(mem.getValue());
    iBrickLayout = static_cast<uint32_t>(blayout.getValue());
    iCompression = static_cast<uint32_t>(compression.getValue());
    iCompressionLevel = static_cast<uint32_t>(complevel.getValue());

    bhierarchical = hierarchical.getValue();
    bCreateFile = create.getValue();
    bVerify = !noverify.getValue();
    bShow1dhist = hist1d.getValue();
    bShow2dhist = hist2d.getValue();
    eCreationType = ECreationType(ctype.getValue());
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

  if (iCompression && !bUseToCBlock) {
    cerr << endl << "Brick compression is not available with the "
                    "old file format (-r switch)" << endl;
    return EXIT_FAILURE;
  }

  if (iIter && (!bCreateFile || eCreationType != CT_FRACTAL)) {
    cerr << endl << "Iteration count only valid when computing a mandelbuld "
                    "fractal in file creation mode" << endl;
    return EXIT_FAILURE;
  }

  if (bCreateFile) {

    if (iMem == 0)
     iMem = uint32_t(Controller::Instance().SysInfo()->GetMaxUsableCPUMem()/(1024*1024*1024));
    MESSAGE("Using up to %u GB RAM", iMem);
    cout << endl;

    if (!CreateUVFFile(strUVFName, vSize, iBitSize, eCreationType, iIter,
                       bUseToCBlock, bKeepRaw, iCompression, iMem, iBrickSize,
                       iBrickLayout, iCompressionLevel, bhierarchical))
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
