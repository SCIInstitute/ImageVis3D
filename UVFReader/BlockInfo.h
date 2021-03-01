#ifndef BLOCKINFO_H
#define BLOCKINFO_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>

#include "../Tuvok/StdTuvokDefines.h"
#include "../Tuvok/Controller/Controller.h"

#include "../Tuvok/IO/IOManager.h"
// #include "../Tuvok/IO/TuvokSizes.h"
#include "../Tuvok/IO/UVF/UVF.h"
#include "../Tuvok/IO/UVF/Histogram1DDataBlock.h"
#include "../Tuvok/IO/UVF/Histogram2DDataBlock.h"
#include "../Tuvok/IO/UVF/MaxMinDataBlock.h"
#include "../Tuvok/IO/UVF/RasterDataBlock.h"
#include "../Tuvok/IO/UVF/TOCBlock.h"
#include "../Tuvok/IO/UVF/KeyValuePairDataBlock.h"
#include "../Tuvok/IO/UVF/GeometryDataBlock.h"

using namespace std;

void PrintGeneralBlockInfo(const DataBlock* b, uint64_t i) {
  cout << "    Block " << i << ": " << b->strBlockID
        << endl
        << "      Data is of type: "
        << UVFTables::BlockSemanticTableToCharString(b->GetBlockSemantic())
        << endl
        << "      Global Block Compression is : "
        << UVFTables::CompressionSemanticToCharString(b->ulCompressionScheme)
        << endl;
}

void PrintToCBlockInfo(const TOCBlock* b) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

  cout << "      Volume Information: " << endl
        << "        Level of detail: " << b->GetLoDCount() << endl
        << "        Max Bricksize: (" << b->GetMaxBrickSize().x << " x "
                                      << b->GetMaxBrickSize().y << " x "
                                      << b->GetMaxBrickSize().z << ")" << endl;
  for (uint64_t i=0;i<b->GetLoDCount();++i) {
    cout << "          Level " << i << " size:" << b->GetLODDomainSize(i).x <<
                                            "x" << b->GetLODDomainSize(i).y <<
                                            "x" << b->GetLODDomainSize(i).z <<
                                            endl;
    
    UINT64VECTOR3 brickCount = b->GetBrickCount(i);
    cout << "            Bricks: " << brickCount.x << 
                               "x" << brickCount.y << 
                               "x" << brickCount.z;

    uint64_t iCompressionNone = 0;
    uint64_t iCompressionZLIB = 0;
    uint64_t iCompressionLZMA = 0;
    uint64_t iCompressionLZ4 = 0;
    uint64_t iCompressionBZLIB = 0;
    uint64_t iCompressionLZHAM = 0;
    uint64_t iCompressionOther = 0;

    for (uint64_t bz=0;bz<brickCount.z;++bz) {
      for (uint64_t by=0;by<brickCount.y;++by) {
        for (uint64_t bx=0;bx<brickCount.x;++bx) {
          const TOCEntry& te = b->GetBrickInfo(UINT64VECTOR4(bx,by,bz,i));

          switch (te.m_eCompression) {
            case CT_NONE  : iCompressionNone++; break;
            case CT_ZLIB  : iCompressionZLIB++; break;
            case CT_LZMA  : iCompressionLZMA++; break;
            case CT_LZ4   : iCompressionLZ4++; break;
            case CT_BZLIB : iCompressionBZLIB++; break;
            case CT_LZHAM : iCompressionLZHAM++; break;
            default : iCompressionOther++; break;
          }
        }
      }
    }
    cout << " (";
    if (iCompressionNone) cout << " Uncompressed:" << iCompressionNone;
    if (iCompressionZLIB) cout <<" ZLIB:" << iCompressionZLIB;
    if (iCompressionLZMA) cout <<" LZMA:" << iCompressionLZMA;
    if (iCompressionLZ4) cout <<" LZ4:" << iCompressionLZ4;
    if (iCompressionBZLIB) cout <<" BZLIB:" << iCompressionBZLIB;
    if (iCompressionLZHAM) cout <<" LZHAM:" << iCompressionLZHAM;
    if (iCompressionOther) cout <<" Other:" << iCompressionOther;
    cout << " )" << endl;

  }
}

void PrintRDBlockInfo(const RasterDataBlock* b, bool bShowData) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

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
            << UVFTables::ElementSemanticTableToCharString(
                                b->ulElementSemantic[j][k]).c_str();
    }
    cout << endl
          << "        Transformation:\n";
    size_t ulTransformDimension = b->ulDomainSemantics.size()+1;
    if (ulTransformDimension * ulTransformDimension !=
        b->dDomainTransformation.size()) {
      cerr << "      error in domain transformation: " << endl;
      return;
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

    uint64_t bit_width = b->ulElementBitSize[0][0];
    const bool is_signed = b->bSignedElement[0][0];
    const bool is_float = bit_width != b->ulElementMantissa[0][0];
    if(is_float && bit_width == 32) {
      std::copy(LODBrickIterator<float, FINEST_RESOLUTION>(b),
                LODBrickIterator<float, FINEST_RESOLUTION>(),
                std::ostream_iterator<float>(std::cout, " "));
    } else if(is_float && bit_width == 64) {
      std::copy(LODBrickIterator<double, FINEST_RESOLUTION>(b),
                LODBrickIterator<double, FINEST_RESOLUTION>(),
                std::ostream_iterator<double>(std::cout, " "));
    } else if(!is_signed && bit_width ==  8) {
      // note for this and uint8 we use "int", but only in the output
      // iterator -- otherwise the stream interprets it as character
      // data and outputs garbage.
      std::copy(LODBrickIterator<uint8_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<uint8_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<int>(std::cout, " "));
    } else if( is_signed && bit_width ==  8) {
      std::copy(LODBrickIterator<int8_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<int8_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<int>(std::cout, " "));
    } else if(!is_signed && bit_width == 16) {
      std::copy(LODBrickIterator<uint16_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<uint16_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<uint16_t>(std::cout, " "));
    } else if( is_signed && bit_width == 16) {
      std::copy(LODBrickIterator<int16_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<int16_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<int16_t>(std::cout, " "));
    } else if(!is_signed && bit_width == 32) {
      std::copy(LODBrickIterator<uint32_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<uint32_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<uint32_t>(std::cout, " "));
    } else if( is_signed && bit_width == 32) {
      std::copy(LODBrickIterator<int32_t, FINEST_RESOLUTION>(b),
                LODBrickIterator<int32_t, FINEST_RESOLUTION>(),
                std::ostream_iterator<int32_t>(std::cout, " "));
    } else {
      T_ERROR("Unsupported data type!");
    }
    std::cout << "\n";
  }
}

void PrintKVPBlockInfo(const KeyValuePairDataBlock* b) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

  cout << "      Data size: " << b->ComputeDataSize() << "\n"
        << "      Values (" << b->GetKeyCount() << "): " << endl;

  for (size_t i = 0;i<b->GetKeyCount();i++) {
    cout << "        " << SysTools::toNarrow(b->GetKeyByIndex(i)).c_str() << " -> "
          << SysTools::toNarrow(b->GetValueByIndex(i)).c_str() << endl;
  }
}

void PrintH1DBlockInfo(const Histogram1DDataBlock* b, bool bShow1dhist) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

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

void PrintH2DBlockInfo(const Histogram2DDataBlock* b, bool bShow2dhist) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

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

void PrintMaxMinBlockInfo(const MaxMinDataBlock* b) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

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
}

void PrintGeoBlockInfo(const GeometryDataBlock* b) {
  if (!b) {
    cerr << "Block cast error" << endl;
    return;
  }

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

bool DisplayUVFInfo(const std::wstring& strUVFName, bool bVerify, bool bShowData, 
                    bool bShow1dhist, bool bShow2dhist) {
  UVF uvfFile(strUVFName);
  std::string strProblem;
  if (!uvfFile.Open(false, bVerify, false, &strProblem)) {
    cerr << endl << "Unable to open file " << SysTools::toNarrow(strUVFName).c_str() << "!"
          << endl << "Error: " << strProblem.c_str() << endl;
    return false;
  }

  cout << "Successfully opened UVF File " << SysTools::toNarrow(strUVFName).c_str() << endl;
  const GlobalHeader& gh = uvfFile.GetGlobalHeader();

  if (gh.bIsBigEndian) {
    cout << "  File is BIG endian format!" << endl;
  } else {
    cout << "  File is little endian format!" << endl;
  }

  cout << "  The version of the file is "
        << gh.ulFileVersion
        << " (the version of the reader is " << UVF::ms_ulReaderVersion
        << ")"<< endl
        << "  The file uses the "
        << UVFTables::ChecksumSemanticToCharString(
                                gh.ulChecksumSemanticsEntry).c_str()
        << " checksum technology with a bitlength of "
        << gh.vcChecksum.size()*8;
  if (gh.ulChecksumSemanticsEntry > UVFTables::CS_NONE &&
      gh.ulChecksumSemanticsEntry < UVFTables::CS_UNKNOWN)
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

  if (gh.ulAdditionalHeaderSize > 0) {
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
    const DataBlock* b = uvfFile.GetDataBlock(i).get();
    PrintGeneralBlockInfo(b, i);
    switch (b->GetBlockSemantic()) {
      case UVFTables::BS_TOC_BLOCK:
        PrintToCBlockInfo(dynamic_cast<const TOCBlock*>(b));
        break;
      case UVFTables::BS_REG_NDIM_GRID :
        PrintRDBlockInfo(dynamic_cast<const RasterDataBlock*>(b),
                          bShowData);
        break;
      case UVFTables::BS_KEY_VALUE_PAIRS :
        PrintKVPBlockInfo(dynamic_cast<const KeyValuePairDataBlock*>(b));
        break;
      case UVFTables::BS_1D_HISTOGRAM:
        PrintH1DBlockInfo(dynamic_cast<const Histogram1DDataBlock*>(b),
                            bShow1dhist);
        break;
      case UVFTables::BS_2D_HISTOGRAM:
        PrintH2DBlockInfo(dynamic_cast<const Histogram2DDataBlock*>(b),
                            bShow2dhist);
        break;
      case UVFTables::BS_MAXMIN_VALUES:
        PrintMaxMinBlockInfo(dynamic_cast<const MaxMinDataBlock*>(b));
        break;
      case UVFTables::BS_GEOMETRY:
        PrintGeoBlockInfo(dynamic_cast<const GeometryDataBlock*>(b));
        break;
      case UVFTables::BS_EMPTY:
        break;
      default:
        /// \todo handle other block types
        T_ERROR("    -->  Unknown/Unimplemented block type %d",
                static_cast<int>(b->GetBlockSemantic()));
        break;
    }
  }

  uvfFile.Close();

  return true;
}


#endif // BLOCKINFO_H

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
