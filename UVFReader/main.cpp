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
//!    Copyright (C) 2009 IVDA, MMC, DFKI, SCI Institute

#include "../Tuvok/StdTuvokDefines.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../CmdLineConverter/DebugOut/HRConsoleOut.h"

#include "../Tuvok/IO/IOManager.h"
#include "../Tuvok/IO/UVF/UVF.h"
#include "../Tuvok/IO/UVF/Histogram1DDataBlock.h"
#include "../Tuvok/IO/UVF/Histogram2DDataBlock.h"
#include "../Tuvok/IO/UVF/MaxMinDataBlock.h"
#include "../Tuvok/IO/UVF/RasterDataBlock.h"
#include "../Tuvok/IO/UVF/KeyValuePairDataBlock.h"
#include "../Tuvok/IO/UVF/GeometryDataBlock.h"


#include <string>
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;
using namespace tuvok;

#define READER_VERSION 1.1

#ifdef _WIN32
  // CRT's memory leak detection
  #if defined(DEBUG) || defined(_DEBUG)
    #include <crtdbg.h>
  #endif
#endif

void ShowUsage(string filename) {
  cout << endl <<
   filename << " V" << READER_VERSION << " (using Tuvok V" << TUVOK_VERSION <<
   " " << TUVOK_VERSION_TYPE << ")" << endl << endl <<
   " Reads, verifies, and creates UVF Files" << endl << endl <<
   " Usage:" << endl <<
   "  " << filename << " -f File.uvf [-noverify -create [-lod UINT]\n"
   "            [-sizeX UINT] [-sizeY UINT] [-sizeZ UINT]] " << endl << endl <<
   "     Mandatory Arguments:" << endl <<
   "         -f       the filename of the UVF file to process" << endl <<
   "     Optional Arguments:" << endl <<
   "        -noverify    disables the checksum test" << endl <<
   "        -show1dhist  also output 1D histogram to console" << endl <<
   "        -show2dhist  also output 2D histogram to console" << endl <<
   "        -create      if set create a new test UVF file with a\n"
   "                     filename set by -f" << endl <<
   "        -sizeX       requires '-create' argument, specifies the width\n"
   "                     of the volume to be created (default = 100)"<< endl <<
   "        -sizeY       requires '-create' argument, specifies the height\n"
   "                     of the volume to be created (default = 200)"<< endl <<
   "        -sizeZ       requires '-create' argument, specifies the depth\n"
   "                     of the volume to be created (default = 300)"<< endl <<
   "        -bits        requires '-create' argument, specifies the bit depth\n"
   "                     of the volume, may be 8, 16 (default = 8)" << endl <<
   "        -brickSize   requires '-create' argument, specifies the maximum\n"
   "                     bricksize (default = " << DEFAULT_BRICKSIZE <<")"
   << endl;
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

  int iSizeX = 100;
  int iSizeY = 200;
  int iSizeZ = 300;
  int iBitSize = 8;
  unsigned int iBrickSize = DEFAULT_BRICKSIZE;

  SysTools::CmdLineParams comLine(argc,argv);
  string strFilename = SysTools::GetFilename(argv[0]);

  comLine.GetValue("F", strUVFName);
  comLine.GetValue("SIZEX", iSizeX);
  comLine.GetValue("SIZEY", iSizeY);
  comLine.GetValue("SIZEZ", iSizeZ);
  comLine.GetValue("BITS", iBitSize);
  comLine.GetValue("BRICKSIZE", iBrickSize);

  VECTOR3<UINT64> iSize = VECTOR3<UINT64>(UINT64(iSizeX),UINT64(iSizeY),UINT64(iSizeZ));

  bool bCreateFile = comLine.SwitchSet("CREATE");
  bool bVerify     = !comLine.SwitchSet("NOVERIFY");
  bool bShow1dhist = comLine.SwitchSet("SHOW1DHIST");
  bool bShow2dhist = comLine.SwitchSet("SHOW2DHIST");

  if (strUVFName == "") {
    cerr << endl << "Missing Argument -f or filename was empty" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  if (!bCreateFile && comLine.SwitchSet("SIZEX")) {
    cerr << endl << "Argument -sizeX requires -create" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  if (!bCreateFile && comLine.SwitchSet("SIZEY")) {
    cerr << endl << "Argument -sizeY requires -create" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  if (!bCreateFile && comLine.SwitchSet("SIZEZ")) {
    cerr << endl << "Argument -sizeZ requires -create" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  if (!bCreateFile && comLine.SwitchSet("BITS")) {
    cerr << endl << "Argument -bits requires -create" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  if (iBitSize != 8 && iBitSize != 16) {
    cerr << endl << "Argument -bits can only be 8 or 16" << endl;
    ShowUsage(strFilename);
    return EXIT_FAILURE;
  }

  wstring wstrUVFName(strUVFName.begin(), strUVFName.end());
  UVF uvfFile(wstrUVFName);

  if (bCreateFile) {
    MESSAGE("Preparing creation of UVF file %s", strUVFName.c_str());

    GlobalHeader uvfGlobalHeader;
    uvfGlobalHeader.ulChecksumSemanticsEntry = UVFTables::CS_MD5;
    uvfFile.SetGlobalHeader(uvfGlobalHeader);

    DataBlock testBlock;
    testBlock.strBlockID = "Test Block 1";
    testBlock.ulCompressionScheme = UVFTables::COS_NONE;
    uvfFile.AddDataBlock(&testBlock,0);

    testBlock.strBlockID = "Test Block 2";
    uvfFile.AddDataBlock(&testBlock,0);

    RasterDataBlock testVolume;

    testVolume.strBlockID = "Test Volume 1";

    testVolume.ulCompressionScheme = UVFTables::COS_NONE;
    testVolume.ulDomainSemantics.push_back(UVFTables::DS_X);
    testVolume.ulDomainSemantics.push_back(UVFTables::DS_Y);
    testVolume.ulDomainSemantics.push_back(UVFTables::DS_Z);

    testVolume.ulDomainSize.push_back(iSize.x);
    testVolume.ulDomainSize.push_back(iSize.y);
    testVolume.ulDomainSize.push_back(iSize.z);

    testVolume.ulLODDecFactor.push_back(2);
    testVolume.ulLODDecFactor.push_back(2);
    testVolume.ulLODDecFactor.push_back(2);

    testVolume.ulLODGroups.push_back(0);
    testVolume.ulLODGroups.push_back(0);
    testVolume.ulLODGroups.push_back(0);

    UINT64 iLodLevelCount = 1;
    UINT32 iMaxVal = UINT32(iSize.maxVal());

    while (iMaxVal > iBrickSize) {
      iMaxVal /= 2;
      iLodLevelCount++;
    }

    testVolume.ulLODLevelCount.push_back(iLodLevelCount);

    testVolume.SetTypeToScalar(iBitSize,iBitSize,false,UVFTables::ES_CT);

    testVolume.ulBrickSize.push_back(iBrickSize);
    testVolume.ulBrickSize.push_back(iBrickSize);
    testVolume.ulBrickSize.push_back(iBrickSize);

    testVolume.ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP);
    testVolume.ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP);
    testVolume.ulBrickOverlap.push_back(DEFAULT_BRICKOVERLAP);

    vector<double> vScale;
    vScale.push_back(double(iSize.maxVal())/double(iSize.x));
    vScale.push_back(double(iSize.maxVal())/double(iSize.y));
    vScale.push_back(double(iSize.maxVal())/double(iSize.z));
    testVolume.SetScaleOnlyTransformation(vScale);

    MaxMinDataBlock MaxMinData(1);

    MESSAGE("Generating dummy data");

    switch (iBitSize) {
      case 8 : {
                  std::vector<unsigned char> source(size_t(iSize.volume()));
                  size_t i = 0;
                  for (UINT64 z = 0;z<iSize.z;z++)
                    for (UINT64 y = 0;y<iSize.y;y++)
                      for (UINT64 x = 0;x<iSize.x;x++) {
                        source[i++] = static_cast<unsigned char>(std::max(0.0f,(0.5f-(0.5f-FLOATVECTOR3(float(x),float(y),float(z))/FLOATVECTOR3(iSize)).length())*512.0f));
                      }

                  if (!testVolume.FlatDataToBrickedLOD(&source[0], "./tempFile.tmp", CombineAverage<unsigned char,1>, SimpleMaxMin<unsigned char,1>, &MaxMinData, &Controller::Debug::Out())){
                    T_ERROR("Failed to subdivide the volume into bricks");
                    uvfFile.Close();
                    return EXIT_FAILURE;
                  }
                  break;
               }
      case 16 :{
                  std::vector<unsigned short> source(size_t(iSize.volume()));
                  size_t i = 0;
                  for (UINT64 z = 0;z<iSize.z;z++)
                    for (UINT64 y = 0;y<iSize.y;y++)
                      for (UINT64 x = 0;x<iSize.x;x++) {
                        source[i++] = static_cast<unsigned short>(std::max(0.0f,(0.5f-(0.5f-FLOATVECTOR3(float(x),float(y),float(z))/FLOATVECTOR3(iSize)).length())*131072.0f));
                      }

                  if (!testVolume.FlatDataToBrickedLOD(&source[0], "./tempFile.tmp", CombineAverage<unsigned short,1>, SimpleMaxMin<unsigned short,1>, &MaxMinData, &Controller::Debug::Out())){
                    T_ERROR("Failed to subdivide the volume into bricks");
                    uvfFile.Close();
                    return EXIT_FAILURE;
                  }
                  break;
               }
      default: assert(0); // should never happen as we test this during parameter check
    }

    string strProblemDesc;
    if (!testVolume.Verify(&strProblemDesc)) {
      T_ERROR("Verify failed with the following reason: %s", strProblemDesc.c_str());
      uvfFile.Close();
      return EXIT_FAILURE;
    }

    if (!uvfFile.AddDataBlock(&testVolume,testVolume.ComputeDataSize(), true)) {
      T_ERROR("AddDataBlock failed!");
      uvfFile.Close();
      return EXIT_FAILURE;
    }

    MESSAGE("Computing 1D Histogram...");
    Histogram1DDataBlock Histogram1D;
    if (!Histogram1D.Compute(&testVolume)) {
      T_ERROR("Computation of 1D Histogram failed!");
      uvfFile.Close();
      return EXIT_FAILURE;
    }

    MESSAGE("Computing 2D Histogram...");
    Histogram2DDataBlock Histogram2D;
    if (!Histogram2D.Compute(&testVolume, Histogram1D.GetHistogram().size(), MaxMinData.m_GlobalMaxMin.maxScalar)) {
      T_ERROR("Computation of 2D Histogram failed!");
      uvfFile.Close();
      return EXIT_FAILURE;
    }
    MESSAGE("Storing histogram data...");
    uvfFile.AddDataBlock(&Histogram1D,Histogram1D.ComputeDataSize());
    uvfFile.AddDataBlock(&Histogram2D,Histogram2D.ComputeDataSize());

    MESSAGE("Storing acceleration data...");
    uvfFile.AddDataBlock(&MaxMinData, MaxMinData.ComputeDataSize());

    MESSAGE("Storing metadata...");

    KeyValuePairDataBlock metaPairs;
    metaPairs.AddPair("Data Source","This file was created by the UVFReader");
    metaPairs.AddPair("Decription","Dummy file for testing purposes.");

    if (EndianConvert::IsLittleEndian())
      metaPairs.AddPair("Source Endianess","little");
    else
      metaPairs.AddPair("Source Endianess","big");

    metaPairs.AddPair("Source Type","integer");
    metaPairs.AddPair("Source Bitwidth",SysTools::ToString(iBitSize));

    UINT64 iDataSize = metaPairs.ComputeDataSize();
    uvfFile.AddDataBlock(&metaPairs,iDataSize);

    MESSAGE("Writing UVF file...");

    if (!uvfFile.Create()) {
      T_ERROR("Failed to create UVF file %s", strUVFName.c_str());
      return EXIT_FAILURE;
    }

    MESSAGE("Computing checksum...");

    uvfFile.Close();

    MESSAGE("Sucesfully created UVF file %s", strUVFName.c_str());

  } else {
    std::string strProblem;
    if (!uvfFile.Open(false, bVerify, false, &strProblem)) {
      cerr << endl << "Unable to open file " << strUVFName.c_str() << "!"
           << endl << "Error: " << strProblem.c_str() << endl;
      return -2;
    }

    cout << "Sucessfully opened UVF File " << strUVFName.c_str() << endl;

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
      cout << "  Futher (unparsed) global header information was found!!! "
           << endl;
    }
    if (uvfFile.GetDataBlockCount() ==  1) {
      cout << "  It contains one block of data" << endl;
    } else {
      cout << "  It contains " << uvfFile.GetDataBlockCount()
           << " blocks of data" << endl;
    }

    for(UINT64 i = 0; i<uvfFile.GetDataBlockCount(); i++) {
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
            size_t j = 0;
            for (size_t y = 0;y<ulTransformDimension;y++) {
              cout << "        ";
              for (size_t x = 0;x<ulTransformDimension;x++) {
                cout << " " << b->dDomainTransformation[j++];
              }
              cout << endl;
            }
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
        case UVFTables::BS_MAXMIN_VALUES:
          /// @todo FIXME: implement :)
          cout << "      Query of MaxMin data block info is unimplemented.\n";
          break;
        case UVFTables::BS_GEOMETRY: {
            const GeometryDataBlock* b = (const GeometryDataBlock*)uvfFile.GetDataBlock(i);

            cout << "      Descripton: " << b->m_Desc.c_str() << ".\n";

            size_t vI = b->GetVertexIndices().size()/3;
            size_t vN = b->GetNormalIndices().size()/3;
            size_t vT = b->GetTexCoordIndices().size()/2;
            size_t vC = b->GetColorIndices().size()/4;
            size_t v = b->GetVertices().size()/b->GetPolySize();
            size_t n = b->GetNormals().size()/b->GetPolySize();
            size_t t = b->GetTexCoords().size()/b->GetPolySize();
            size_t c = b->GetColors().size()/b->GetPolySize();

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
              cout << " (no used since vertex colors are specified)\n";
            else 
              cout << "\n";

          }
        break;
        default:
          /// \todo handle other block types
          T_ERROR("Unknown block type %d",
                  static_cast<int>(uvfFile.GetDataBlock(i)->GetBlockSemantic()));
          break;
      }
    }

    uvfFile.Close();
  }

  return EXIT_SUCCESS;
}
