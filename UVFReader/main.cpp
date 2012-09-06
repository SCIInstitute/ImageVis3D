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

//!    File   : main.cpp
//!    Author : Jens Krueger
//!             IVDA, MMCI, DFKI Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : October 2009
//
//!    Copyright (C) 2009 IVDA, MMCI, DFKI, SCI Institute

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cmath>

#include <tclap/CmdLine.h>

#include "../Tuvok/StdTuvokDefines.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../CmdLineConverter/DebugOut/HRConsoleOut.h"

#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/TuvokSizes.h"
#include "../Tuvok/IO/UVF/UVF.h"
#include "../Tuvok/IO/UVF/Histogram1DDataBlock.h"
#include "../Tuvok/IO/UVF/Histogram2DDataBlock.h"
#include "../Tuvok/IO/UVF/MaxMinDataBlock.h"
#include "../Tuvok/IO/UVF/RasterDataBlock.h"
#include "../Tuvok/IO/UVF/TOCBlock.h"
#include "../Tuvok/IO/UVF/KeyValuePairDataBlock.h"
#include "../Tuvok/IO/UVF/GeometryDataBlock.h"

using namespace boost;
using namespace std;
using namespace tuvok;

#define READER_VERSION 2.0

#ifdef _WIN32
  // CRT's memory leak detection
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif

double radius(double x, double y, double z)
{
  return std::sqrt(x*x + y*y + z*z);
}

double phi(double x, double y)
{
  return std::atan2(y, x);
}

double theta(double x, double y, double z)
{
  return std::atan2(std::sqrt(x*x + y*y), z);
}

double PowerX(double x, double y, double z, double cx, int n, double power)
{
  return cx + power*std::sin(phi(x,y)*n)*std::cos(theta(x,y,z)*n);
}

double PowerY(double x, double y, double z, double cy, int n, double power)
{
  return cy + power*std::sin(phi(x,y)*n)*std::sin(theta(x,y,z)*n);
}

double PowerZ(double x, double y, double cz, int n, double power)
{
  return cz + power*std::cos(phi(x,y)*n);
}

double ComputeMandelbulb(const double sx, const double sy, const double sz, const uint32_t n, const uint32_t iMaxIterations, const double fBailout) {

  double fx = sx;
  double fy = sy;
  double fz = sz;
  double r = radius(fx, fy, fz);

  for (uint32_t i = 0; i < iMaxIterations; i++) {

    const double fPower = std::pow(r, static_cast<double>(n));

    const double fx_ = PowerX(fx, fy, fz, sx, n, fPower);
    const double fy_ = PowerY(fx, fy, fz, sy, n, fPower);
    const double fz_ = PowerZ(fx, fy    , sz, n, fPower);

    fx = fx_;
    fy = fy_;
    fz = fz_;

    if ((r = radius(fx, fy, fz)) > fBailout)
      return static_cast<double>(i) / iMaxIterations;
  }

  return 1.0;
}

template<typename T, bool bMandelbulb> void GenerateVolumeData(UINT64VECTOR3 vSize, LargeRAWFile_ptr pDummyData) {
  T* source = new T[size_t(vSize.x)];

  for (uint64_t z = 0;z<vSize.z;z++) {
    MESSAGE("Generating Data %.3f%%", 100.0*(double)z/vSize.z);
    for (uint64_t y = 0;y<vSize.y;y++) {
#pragma omp parallel for 
      for (int64_t x = 0;x<int64_t(vSize.x);x++) {
        if (bMandelbulb)
          source[x] = static_cast<T>(ComputeMandelbulb(3.0 * static_cast<double>(x)/(vSize.x-1) - 1.5,
                                                       3.0 * static_cast<double>(y)/(vSize.y-1) - 1.5,
                                                       3.0 * static_cast<double>(z)/(vSize.z-1) - 1.5,
                                                       8, std::numeric_limits<T>::max(), 4.0) * std::numeric_limits<T>::max());
        else
          source[x] = static_cast<T>(std::max(0.0f,(0.5f-(0.5f-FLOATVECTOR3(float(x),float(y),float(z))/FLOATVECTOR3(vSize)).length())*std::numeric_limits<T>::max()*2));
      }
      pDummyData->WriteRAW((uint8_t*)source, vSize.x*sizeof(T));
    }
  }

  delete [] source;
}

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

  size_t iSizeX = 100;
  size_t iSizeY = 200;
  size_t iSizeZ = 300;
  size_t iBitSize = 8;
  unsigned int iBrickSize = DEFAULT_BRICKSIZE;
  bool bCreateFile;
  bool bVerify;
  bool bShow1dhist;
  bool bShow2dhist;
  bool bMandelbulb;
  bool bShowData;
  bool bUseToCBlock;

  try {
    TCLAP::CmdLine cmd("UVF diagnostic tool");
    TCLAP::MultiArg<std::string> inputs("i", "input", "input file.",
                                        true, "filename");
    TCLAP::SwitchArg noverify("n", "noverify", "disable the checksum test",
                              false);
    TCLAP::SwitchArg hist1d("1", "1dhist", "output the 1D histogram", false);
    TCLAP::SwitchArg hist2d("2", "2dhist", "output the 2D histogram", false);
    TCLAP::SwitchArg create("c", "create", "create instead of read a UVF",
                            false);
    TCLAP::SwitchArg mandelbulb("m", "mandelbulb", "compute mandelbulb fractal instead of simple sphere", false);
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

    cmd.add(inputs);
    cmd.add(noverify);
    cmd.add(hist1d);
    cmd.add(hist2d);
    cmd.add(create);
    cmd.add(mandelbulb);
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
    iSizeX = sizeX.getValue();
    iSizeY = sizeY.getValue();
    iSizeZ = sizeZ.getValue();
    iBitSize = bits.getValue();
    iBrickSize = static_cast<unsigned>(bsize.getValue());

    bCreateFile = create.getValue();
    bVerify = !noverify.getValue();
    bShow1dhist = hist1d.getValue();
    bShow2dhist = hist2d.getValue();
    bMandelbulb = mandelbulb.getValue();
    bShowData = output_data.getValue();
    bUseToCBlock = !use_rdb.getValue();
  } catch(const TCLAP::ArgException& e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << "\n";
    return EXIT_FAILURE;
  }

  VECTOR3<uint64_t> vSize = VECTOR3<uint64_t>(uint64_t(iSizeX),uint64_t(iSizeY),uint64_t(iSizeZ));

  if (strUVFName.empty()) {
    cerr << endl << "Missing Argument -f or filename was empty" << endl;
    return EXIT_FAILURE;
  }

  if (iBitSize != 8 && iBitSize != 16) {
    cerr << endl << "Argument -bits can only be 8 or 16" << endl;
    return EXIT_FAILURE;
  }

  wstring wstrUVFName(strUVFName.begin(), strUVFName.end());
  UVF uvfFile(wstrUVFName);

  if (bCreateFile) {
    MESSAGE("Generating dummy data");

    LargeRAWFile_ptr dummyData = LargeRAWFile_ptr(new LargeRAWFile("./dummyData.raw"));
    if (!dummyData->Create(vSize.volume()*iBitSize/8)) {
      T_ERROR("Failed to create ./dummyData.raw file");
      return EXIT_FAILURE;
    }

    switch (iBitSize) {
      case 8 :
        if (bMandelbulb)
          GenerateVolumeData<uint8_t, true>(vSize, dummyData);
        else
          GenerateVolumeData<uint8_t, false>(vSize, dummyData);
        break;
      case 16 :
        if (bMandelbulb)
          GenerateVolumeData<uint16_t, true>(vSize, dummyData);
        else
          GenerateVolumeData<uint16_t, false>(vSize, dummyData);
        break;
      default:
        T_ERROR("Invalid bitsize");
        return EXIT_FAILURE;
    }
    dummyData->Close();


    MESSAGE("Preparing creation of UVF file %s", strUVFName.c_str());

    GlobalHeader uvfGlobalHeader;
    uvfGlobalHeader.ulChecksumSemanticsEntry = UVFTables::CS_MD5;
    uvfFile.SetGlobalHeader(uvfGlobalHeader);

    std::shared_ptr<DataBlock> testBlock(new DataBlock());
    testBlock->strBlockID = "Test Block 1";
    testBlock->ulCompressionScheme = UVFTables::COS_NONE;
    uvfFile.AddDataBlock(testBlock);

    testBlock = std::shared_ptr<DataBlock>(new DataBlock());
    testBlock->strBlockID = "Test Block 2";
    uvfFile.AddDataBlock(testBlock);

    std::shared_ptr<DataBlock> pTestVolume;
    std::shared_ptr<MaxMinDataBlock> MaxMinData(
      new MaxMinDataBlock(1)
    );
    std::shared_ptr<RasterDataBlock> testRasterVolume(
      new RasterDataBlock()
    );
    std::shared_ptr<TOCBlock> tocBlock(new TOCBlock());

    if (bUseToCBlock)  {
      tocBlock->strBlockID = "Test TOC Volume 1";
      tocBlock->ulCompressionScheme = UVFTables::COS_NONE;

      bool bResult = tocBlock->FlatDataToBrickedLOD("./dummyData.raw",
        "./tempFile.tmp", iBitSize == 8 ? ExtendedOctree::CT_UINT8
                                        : ExtendedOctree::CT_UINT16,
        1, vSize, DOUBLEVECTOR3(1,1,1), UINT64VECTOR3(iBrickSize,iBrickSize,iBrickSize),
        DEFAULT_BRICKOVERLAP, false, false,
        1024*1024*1024, MaxMinData,
        &Controller::Debug::Out()
      );

      if (!bResult) {
        T_ERROR("Failed to subdivide the volume into bricks");
        dummyData->Delete();
        uvfFile.Close();
        return EXIT_FAILURE;
      }

      pTestVolume = tocBlock;
    } else {

      testRasterVolume->strBlockID = "Test Volume 1";

      testRasterVolume->ulCompressionScheme = UVFTables::COS_NONE;
      testRasterVolume->ulDomainSemantics.push_back(UVFTables::DS_X);
      testRasterVolume->ulDomainSemantics.push_back(UVFTables::DS_Y);
      testRasterVolume->ulDomainSemantics.push_back(UVFTables::DS_Z);

      testRasterVolume->ulDomainSize.push_back(vSize.x);
      testRasterVolume->ulDomainSize.push_back(vSize.y);
      testRasterVolume->ulDomainSize.push_back(vSize.z);

      testRasterVolume->ulLODDecFactor.push_back(2);
      testRasterVolume->ulLODDecFactor.push_back(2);
      testRasterVolume->ulLODDecFactor.push_back(2);

      testRasterVolume->ulLODGroups.push_back(0);
      testRasterVolume->ulLODGroups.push_back(0);
      testRasterVolume->ulLODGroups.push_back(0);

      uint64_t iLodLevelCount = 1;
      uint32_t iMaxVal = uint32_t(vSize.maxVal());

      while (iMaxVal > iBrickSize) {
        iMaxVal /= 2;
        iLodLevelCount++;
      }

      testRasterVolume->ulLODLevelCount.push_back(iLodLevelCount);

      testRasterVolume->SetTypeToScalar(iBitSize,iBitSize,false,UVFTables::ES_CT);

      testRasterVolume->ulBrickSize.push_back(iBrickSize);
      testRasterVolume->ulBrickSize.push_back(iBrickSize);
      testRasterVolume->ulBrickSize.push_back(iBrickSize);

      testRasterVolume->ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP*2);
      testRasterVolume->ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP*2);
      testRasterVolume->ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP*2);

      vector<double> vScale;
      vScale.push_back(double(vSize.maxVal())/double(vSize.x));
      vScale.push_back(double(vSize.maxVal())/double(vSize.y));
      vScale.push_back(double(vSize.maxVal())/double(vSize.z));
      testRasterVolume->SetScaleOnlyTransformation(vScale);

      dummyData->Open();
      switch (iBitSize) {
      case 8 : {
                    if (!testRasterVolume->FlatDataToBrickedLOD(dummyData, "./tempFile.tmp", CombineAverage<unsigned char,1>, SimpleMaxMin<unsigned char,1>, MaxMinData, &Controller::Debug::Out())){
                      T_ERROR("Failed to subdivide the volume into bricks");
                      uvfFile.Close();
                      dummyData->Delete();
                      return EXIT_FAILURE;
                    }
                    break;
                  }
      case 16 :{
                  if (!testRasterVolume->FlatDataToBrickedLOD(dummyData, "./tempFile.tmp", CombineAverage<unsigned short,1>, SimpleMaxMin<unsigned short,1>, MaxMinData, &Controller::Debug::Out())){
                    T_ERROR("Failed to subdivide the volume into bricks");
                    uvfFile.Close();
                    dummyData->Delete();
                    return EXIT_FAILURE;
                  }
                  break;
                }
      }

      string strProblemDesc;
      if (!testRasterVolume->Verify(&strProblemDesc)) {
        T_ERROR("Verify failed with the following reason: %s",
                strProblemDesc.c_str());
        uvfFile.Close();
        dummyData->Delete();
        return EXIT_FAILURE;
      }

      pTestVolume = testRasterVolume;
    }
    
    dummyData->Delete();

    if (!uvfFile.AddDataBlock(pTestVolume)) {
      T_ERROR("AddDataBlock failed!");
      uvfFile.Close();
      return EXIT_FAILURE;
    }

    std::shared_ptr<Histogram1DDataBlock> Histogram1D(
      new Histogram1DDataBlock()
    );
    std::shared_ptr<Histogram2DDataBlock> Histogram2D(
      new Histogram2DDataBlock()
    );
    if (bUseToCBlock) {
      MESSAGE("Computing 1D Histogram...");
      if (!Histogram1D->Compute(tocBlock.get(), 0)) {
        T_ERROR("Computation of 1D Histogram failed!");
        uvfFile.Close();
        return EXIT_FAILURE;
      }
      Histogram1D->Compress(4096);
      MESSAGE("Computing 2D Histogram...");
      if (!Histogram2D->Compute(tocBlock.get(), 0,
                                Histogram1D->GetHistogram().size(),
                                MaxMinData->GetGlobalValue().maxScalar)) {
        T_ERROR("Computation of 2D Histogram failed!");
        uvfFile.Close();
        return EXIT_FAILURE;
      }
    } else {
      if (!Histogram1D->Compute(testRasterVolume.get())) {
        T_ERROR("Computation of 1D Histogram failed!");
        uvfFile.Close();
        return EXIT_FAILURE;
      }
      Histogram1D->Compress(4096);
      MESSAGE("Computing 2D Histogram...");
      if (!Histogram2D->Compute(testRasterVolume.get(),
                                Histogram1D->GetHistogram().size(),
                                MaxMinData->GetGlobalValue().maxScalar)) {
        T_ERROR("Computation of 2D Histogram failed!");
        uvfFile.Close();
        return EXIT_FAILURE;
      }
    }

    MESSAGE("Storing histogram data...");
    uvfFile.AddDataBlock(Histogram1D);
    uvfFile.AddDataBlock(Histogram2D);

    MESSAGE("Storing acceleration data...");
    uvfFile.AddDataBlock(MaxMinData);

    MESSAGE("Storing metadata...");

    std::shared_ptr<KeyValuePairDataBlock> metaPairs(
      new KeyValuePairDataBlock()
    );
    metaPairs->AddPair("Data Source","This file was created by the UVFReader");
    metaPairs->AddPair("Description","Dummy file for testing purposes.");

    if (EndianConvert::IsLittleEndian())
      metaPairs->AddPair("Source Endianess","little");
    else
      metaPairs->AddPair("Source Endianess","big");

    metaPairs->AddPair("Source Type","integer");
    metaPairs->AddPair("Source Bit width",SysTools::ToString(iBitSize));

    uvfFile.AddDataBlock(metaPairs);

    MESSAGE("Writing UVF file...");

    if (!uvfFile.Create()) {
      T_ERROR("Failed to create UVF file %s", strUVFName.c_str());
      return EXIT_FAILURE;
    }

    MESSAGE("Computing checksum...");

    uvfFile.Close();

    MESSAGE("Successfully created UVF file %s", strUVFName.c_str());

  } else {
    std::string strProblem;
    if (!uvfFile.Open(false, bVerify, false, &strProblem)) {
      cerr << endl << "Unable to open file " << strUVFName.c_str() << "!"
           << endl << "Error: " << strProblem.c_str() << endl;
      return -2;
    }

    cout << "Successfully opened UVF File " << strUVFName.c_str() << endl;

    if (uvfFile.GetGlobalHeader().bIsBigEndian) {
      cout << "  File is BIG endian format!" << endl;
    } else {
      cout << "  File is little endian format!" << endl;
    }
    cout << "  The version of the file is "
         << uvfFile.GetGlobalHeader().ulFileVersion
         << " (the version of the reader is " << UVF::ms_ulReaderVersion
         << ")"<< endl
         << "  The file uses the "
         << UVFTables::ChecksumSemanticToCharString(uvfFile.
                                                    GetGlobalHeader().
                                                    ulChecksumSemanticsEntry).c_str()
         << " checksum technology with a bitlength of "
         << uvfFile.GetGlobalHeader().vcChecksum.size()*8;
    if (uvfFile.GetGlobalHeader().ulChecksumSemanticsEntry > UVFTables::CS_NONE &&
        uvfFile.GetGlobalHeader().ulChecksumSemanticsEntry < UVFTables::CS_UNKNOWN)
    {
      if (bVerify) {
        // since we opened the file with verify, the checksum must be valid
        // if we are at this point :-)
        cout << "  [Checksum is valid!]" << endl;
      } else {
        cout << "  [Checksum not verified by parameter!]" << endl;
      }
    } else {
      cout << endl;
    }

    if (uvfFile.GetGlobalHeader().ulAdditionalHeaderSize > 0) {
      cout << "  further (unparsed) global header information was found!!! "
           << endl;
    }
    if (uvfFile.GetDataBlockCount() ==  1) {
      cout << "  It contains one block of data" << endl;
    } else {
      cout << "  It contains " << uvfFile.GetDataBlockCount()
           << " blocks of data" << endl;
    }

    for(uint64_t i = 0; i<uvfFile.GetDataBlockCount(); i++) {
      cout << "    Block " << i << ": " << uvfFile.GetDataBlock(i)->strBlockID
           << endl
           << "      Data is of type: "
           << UVFTables::BlockSemanticTableToCharString(uvfFile.
                                                        GetDataBlock(i)->
                                                        GetBlockSemantic()).c_str()
           << endl
           << "      Compression is : "
           << UVFTables::CompressionSemanticToCharString(uvfFile.
                                                         GetDataBlock(i)->
                                                         ulCompressionScheme).c_str()
           << endl;

      switch (uvfFile.GetDataBlock(i)->GetBlockSemantic()) {
        case UVFTables::BS_TOC_BLOCK: {
          const TOCBlock* b = (const TOCBlock*)uvfFile.GetDataBlock(i);
          cout << "      Volume Information: " << endl
               << "        Levels of detail: " << b->GetLoDCount() << endl;
          for (size_t i=0;i<size_t(b->GetLoDCount());++i) {
            cout << "          Level " << i << " size:" << b->GetLODDomainSize(i).x << "x" << b->GetLODDomainSize(i).y << "x" << b->GetLODDomainSize(i).z << endl;
            cout << "            Bricks: " << b->GetBrickCount(i).x << "x" << b->GetBrickCount(i).y << "x" << b->GetBrickCount(i).z << endl;
          }
         }
         break;
        case UVFTables::BS_REG_NDIM_GRID : {
          const RasterDataBlock* b = (const RasterDataBlock*)uvfFile.GetDataBlock(i);
          cout << "      Volume Information: " << endl
               << "        Semantics:";
          for (size_t j=0; j < b->ulDomainSemantics.size(); j++) {
            cout << " " << DomainSemanticToCharString(b->ulDomainSemantics[j]).c_str();
          }
          cout << endl
               << "        Levels of detail: "
               << b->ulLODDecFactor.size() << endl
               << "        Size:";
          for (size_t j = 0;j<b->ulDomainSemantics.size();j++) {
            cout << " " << b->ulDomainSize[j];
          }
          cout << endl
               << "        Data:";
          for (size_t j = 0;j<b->ulElementDimension;j++) {
            for (size_t k = 0;k<b->ulElementDimensionSize[j];k++) {
              cout << " "
                   << UVFTables::ElementSemanticTableToCharString(b->ulElementSemantic[j][k]).c_str();
            }
            cout << endl
                 << "        Transformation:\n";
            size_t ulTransformDimension = b->ulDomainSemantics.size()+1;
            if (ulTransformDimension * ulTransformDimension !=
                b->dDomainTransformation.size()) {
              cerr << "      error in domain transformation: " << endl;
              uvfFile.Close();
              return EXIT_FAILURE;
            }
            size_t jj = 0;
            for (size_t y = 0;y<ulTransformDimension;y++) {
              cout << "        ";
              for (size_t x = 0;x<ulTransformDimension;x++) {
                cout << " " << b->dDomainTransformation[jj++];
              }
              cout << endl;
            }
          }
          if(bShowData) {
            std::cout << "        raw data:\n";
            const RasterDataBlock* rdb = dynamic_cast<const RasterDataBlock*>(
              uvfFile.GetDataBlock(i)
            );
            uint64_t bit_width = rdb->ulElementBitSize[0][0];
            const bool is_signed = rdb->bSignedElement[0][0];
            const bool is_float = bit_width != rdb->ulElementMantissa[0][0];
            if(is_float && bit_width == 32) {
              std::copy(LODBrickIterator<float, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<float, FINEST_RESOLUTION>(),
                        std::ostream_iterator<float>(std::cout, " "));
            } else if(is_float && bit_width == 64) {
              std::copy(LODBrickIterator<double, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<double, FINEST_RESOLUTION>(),
                        std::ostream_iterator<double>(std::cout, " "));
            } else if(!is_signed && bit_width ==  8) {
              // note for this and uint8 we use "int", but only in the output
              // iterator -- otherwise the stream interprets it as character
              // data and outputs garbage.
              std::copy(LODBrickIterator<uint8_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<uint8_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<int>(std::cout, " "));
            } else if( is_signed && bit_width ==  8) {
              std::copy(LODBrickIterator<int8_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<int8_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<int>(std::cout, " "));
            } else if(!is_signed && bit_width == 16) {
              std::copy(LODBrickIterator<uint16_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<uint16_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<uint16_t>(std::cout, " "));
            } else if( is_signed && bit_width == 16) {
              std::copy(LODBrickIterator<int16_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<int16_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<int16_t>(std::cout, " "));
            } else if(!is_signed && bit_width == 32) {
              std::copy(LODBrickIterator<uint32_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<uint32_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<uint32_t>(std::cout, " "));
            } else if( is_signed && bit_width == 32) {
              std::copy(LODBrickIterator<int32_t, FINEST_RESOLUTION>(rdb),
                        LODBrickIterator<int32_t, FINEST_RESOLUTION>(),
                        std::ostream_iterator<int32_t>(std::cout, " "));
            } else {
              T_ERROR("Unsupported data type!");
            }
            std::cout << "\n";
          }
        }
        break;
        case UVFTables::BS_KEY_VALUE_PAIRS : {
          const KeyValuePairDataBlock* b =
            (const KeyValuePairDataBlock*)uvfFile.GetDataBlock(i);
          cout << "      Data size: " << b->ComputeDataSize() << "\n"
               << "      Values (" << b->GetKeyCount() << "): " << endl;

          for (size_t i = 0;i<b->GetKeyCount();i++) {
            cout << "        " << b->GetKeyByIndex(i).c_str() << " -> "
                 << b->GetValueByIndex(i).c_str() << endl;
          }
        }
        break;
        case UVFTables::BS_1D_HISTOGRAM: {
          const Histogram1DDataBlock* b = (const Histogram1DDataBlock*)uvfFile.GetDataBlock(i);
          size_t iFilledSize = 0;
          for (size_t i = 0;i<b->GetHistogram().size();i++) {
            if ( b->GetHistogram()[i] != 0) {
              iFilledSize = i+1;
            }
          }
          cout << "      Filled size: " << iFilledSize << endl;
          if (bShow1dhist) {
            cout << "      Entries: " << endl;
            for (size_t i = 0;i<iFilledSize;i++) {
              cout << i << ":" << b->GetHistogram()[i] << " ";
            }
            cout <<  endl;
          }
        }
        break;
        case UVFTables::BS_2D_HISTOGRAM: {
          const Histogram2DDataBlock* b =
            (const Histogram2DDataBlock*)uvfFile.GetDataBlock(i);
          VECTOR2<size_t> vSize(0,0);
          for (size_t j = 0;j<b->GetHistogram().size();j++) {
            for (size_t i = 0;i<b->GetHistogram()[j].size();i++) {
              if ( b->GetHistogram()[j][i] != 0) {
                if ((i+1) > vSize.y) {
                  vSize.y = i+1;
                }
                vSize.x = j+1;
              }
            }
          }
          cout << "      Filled size: " << vSize.x << " x " << vSize.y << endl;
          if (bShow2dhist) {
            cout << "      Entries: " << endl;
            for (size_t j = 0; j < vSize.y; j++) {
              for (size_t i = 0; i < vSize.x; i++) {
                cout << i << "/" << j << ":" << b->GetHistogram()[i][j] << "\n";
              }
            }
            cout << endl;
          }
        }
        break;
        case UVFTables::BS_MAXMIN_VALUES: {
          const MaxMinDataBlock* b = dynamic_cast<const MaxMinDataBlock*>
                                                 (uvfFile.GetDataBlock(i));
          assert(b);

          for (size_t i = 0;i<b->GetComponentCount();++i) {
            if (b->GetComponentCount() > 1) 
              cout << "      Component " << i << ":\n";
            cout << "      Minimum: " << b->GetGlobalValue(i).minScalar << "\n";
            cout << "      Maximum: " << b->GetGlobalValue(i).maxScalar << "\n";
            cout << "      "
                    "Min Gradient: " << b->GetGlobalValue(i).minGradient << "\n";
            cout << "      "
                    "Max Gradient: " << b->GetGlobalValue(i).maxGradient << "\n";
          }
          break;
        }
        case UVFTables::BS_GEOMETRY: {
            const GeometryDataBlock* b = (const GeometryDataBlock*)uvfFile.GetDataBlock(i);

            cout << "      Description: " << b->m_Desc.c_str() << ".\n";

            size_t vI = b->GetVertexIndices().size()/3;
            size_t vN = b->GetNormalIndices().size()/3;
            size_t vT = b->GetTexCoordIndices().size()/2;
            size_t vC = b->GetColorIndices().size()/4;
            size_t v = b->GetVertices().size()/size_t(b->GetPolySize());
            size_t n = b->GetNormals().size()/size_t(b->GetPolySize());
            size_t t = b->GetTexCoords().size()/size_t(b->GetPolySize());
            size_t c = b->GetColors().size()/size_t(b->GetPolySize());

            cout << "      Polygon count: " << vI << ".\n";
            if (vI == vN) cout << "      Valid Normals found.\n";
            if (vI == vT) cout << "      Valid Texture Coordinates found.\n";
            if (vI == vC) cout << "      Valid Colors found.\n";

            cout << "      Vertex count: " << v << ".\n";
            if (n > 0) cout << "      Normal count: " << n << ".\n";
            if (t > 0) cout << "      Texture Coordinate count: " << t << ".\n";
            if (c > 0) cout << "      Color count: " << c << ".\n";

            const std::vector< float >&  col = b->GetDefaultColor();
            cout << "      Default Color: " << col[0] << " "
                 << col[1] << " " << col[2] << " " << col[3];
            if (c > 0)
              cout << " (not used since vertex colors are specified)\n";
            else
              cout << "\n";

          }
        break;
        default:
          /// \todo handle other block types
          T_ERROR("    -->  Unknown block type %d",
                  static_cast<int>(uvfFile.GetDataBlock(i)->GetBlockSemantic()));
          break;
      }
    }

    uvfFile.Close();
  }

  return EXIT_SUCCESS;
}
