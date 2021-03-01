#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdint>

#include "../Tuvok/Controller/Controller.h"

#include "../Tuvok/StdTuvokDefines.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/ProgressTimer.h"
#include "../Tuvok/Basics/Vectors.h"
#include "../Tuvok/Basics/LargeRAWFile.h"
#include "../Tuvok/Basics/SysTools.h"
#include "../CmdLineConverter/DebugOut/HRConsoleOut.h"

#include "../Tuvok/IO/TuvokSizes.h"
#include "../Tuvok/IO/UVF/UVF.h"
#include "../Tuvok/IO/UVF/Histogram1DDataBlock.h"
#include "../Tuvok/IO/UVF/Histogram2DDataBlock.h"
#include "../Tuvok/IO/UVF/MaxMinDataBlock.h"
#include "../Tuvok/IO/UVF/RasterDataBlock.h"
#include "../Tuvok/IO/UVF/TOCBlock.h"
#include "../Tuvok/IO/UVF/KeyValuePairDataBlock.h"

using namespace std;

enum ECreationType {
  CT_FRACTAL,
  CT_CONST_VALUE,
  CT_RANDOM,
  CT_SPHERE
};

static const double bulbSize = 2.25;

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
  return cx + power*
              std::sin(theta(x,y,z)*n)*
              std::cos(phi(x,y)*n);
}

double PowerY(double x, double y, double z,
              double cy, int n, double power)
{
  return cy + power*
              std::sin(theta(x,y,z)*n)*
              std::sin(phi(x,y)*n);
}

double PowerZ(double x, double y, double z,
              double cz, int n, double power)
{
  return cz + power*std::cos(theta(x,y,z)*n);
}

template<typename T>
T ComputeMandelbulb(const double sx, const double sy,
                         const double sz, const uint32_t n,
                         const T iMaxIterations,
                         const double fBailout) {

  double fx = 0;
  double fy = 0;
  double fz = 0;
  double r = radius(fx, fy, fz);

  for (T i = 0; i <= iMaxIterations; i++) {

    const double fPower = std::pow(r, static_cast<double>(n));

    const double fx_ = PowerX(fx, fy, fz, sx, n, fPower);
    const double fy_ = PowerY(fx, fy, fz, sy, n, fPower);
    const double fz_ = PowerZ(fx, fy, fz, sz, n, fPower);

    fx = fx_;
    fy = fy_;
    fz = fz_;

    if ((r = radius(fx, fy, fz)) > fBailout)
      return i;
  }

  return iMaxIterations;
}

template<typename T>
T ComputeMandelbulb(const uint64_t sx, const uint64_t sy,
                    const uint64_t sz, const uint32_t n,
                    const T iMaxIterations,
                    const double fBailout,
                    const UINT64VECTOR3& vTotalSize) {
  return ComputeMandelbulb<T>(bulbSize*double(sx)/(vTotalSize.x-1)-bulbSize/2.0,
                              bulbSize*double(sy)/(vTotalSize.y-1)-bulbSize/2.0,
                              bulbSize*double(sz)/(vTotalSize.z-1)-bulbSize/2.0,
                              n, iMaxIterations, fBailout);
}

template<typename T>
void WriteLineAtOffset(LargeRAWFile_ptr pDummyData, size_t count, size_t pos, T* line) {
  pDummyData->SeekPos(pos*sizeof(T));
  pDummyData->WriteRAW((uint8_t*)line, count*sizeof(T));
}

template<typename T>
void WriteLineAtPos(LargeRAWFile_ptr pDummyData, size_t count,  const UINT64VECTOR3& vOffset, const UINT64VECTOR3& vTotalSize, T* line) {
  const size_t pos = vOffset.x + vOffset.y*vTotalSize.x + vOffset.z*vTotalSize.x*vTotalSize.y;
  WriteLineAtOffset<T>(pDummyData, count, pos, line);
}

template<typename T>
void FillBrick(LargeRAWFile_ptr pDummyData, T value, const UINT64VECTOR3& vOffset, const UINT64VECTOR3& vSize, const UINT64VECTOR3& vTotalSize) {
  std::vector<T> l(vSize.x);
  std::fill(l.begin(), l.end(), value);
  UINT64VECTOR3 vPos = vOffset;
  for (uint64_t z = 0;z<vSize.z;z++) {
    for (uint64_t y = 0;y<vSize.y;y++) {
      vPos.y = vOffset.y + y;
      vPos.z = vOffset.z + z;
      WriteLineAtPos<T>(pDummyData, vSize.x, vPos, vTotalSize, l.data());
    }
  }
}

template<typename T>
bool CheckBlockBoundary(T value, T iIterations, const UINT64VECTOR3& vOffset, const UINT64VECTOR3& vSize, const UINT64VECTOR3& vTotalSize) {
  bool abort = false;
  uint64_t zb = 0;
  for (uint64_t y = 0;y<vSize.y;y++) {
    #pragma omp parallel for
    for (int64_t x = 0;x<int64_t(vSize.x);x++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + x, vOffset.y + y, vOffset.z + zb, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  zb = vSize.z-1;
  for (uint64_t y = 0;y<vSize.y;y++) {
    #pragma omp parallel for
    for (int64_t x = 0;x<int64_t(vSize.x);x++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + x, vOffset.y + y, vOffset.z + zb, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  uint64_t yb = 0;
  for (uint64_t z = 0;z<vSize.z;z++) {
    #pragma omp parallel for
    for (int64_t x = 0;x<int64_t(vSize.x);x++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + x, vOffset.y + yb, vOffset.z + z, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  yb = vSize.y-1;
  for (uint64_t z = 0;z<vSize.z;z++) {
    #pragma omp parallel for
    for (int64_t x = 0;x<int64_t(vSize.x);x++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + x, vOffset.y + yb, vOffset.z + z, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  uint64_t xb = 0;
  for (uint64_t z = 0;z<vSize.z;z++) {
    #pragma omp parallel for
    for (int64_t y = 0;y<int64_t(vSize.y);y++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + xb, vOffset.y + y, vOffset.z + z, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  xb = vSize.x-1;
  for (uint64_t z = 0;z<vSize.z;z++) {
    #pragma omp parallel for
    for (int64_t y = 0;y<int64_t(vSize.y);y++) {
      if ( !abort && ComputeMandelbulb<T>(vOffset.x + xb, vOffset.y + y, vOffset.z + z, 8, iIterations, 100.0, vTotalSize)  != value ) {
        abort = true;
        #pragma omp flush (abort)
      }
    }
  }
  if (abort) return false;

  return true;
}


template<typename T>
void ComputeFractalFast(LargeRAWFile_ptr pDummyData, const UINT64VECTOR3& vOffset, const UINT64VECTOR3& vSize, const UINT64VECTOR3& vTotalSize, const ProgressTimer& timer, uint32_t index=8, T value=0, int depth=1, double completed=0) {

  // compute the eight boundary voxels
  T iIterations = std::numeric_limits<T>::max()-1;

  std::array<T,8> val;
  const std::array<DOUBLEVECTOR3,8> pos = {{
    DOUBLEVECTOR3(bulbSize * (vOffset.x)/(vTotalSize.x-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.y)/(vTotalSize.y-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.z)/(vTotalSize.z-1) - bulbSize/2.0),              // 0

    DOUBLEVECTOR3(bulbSize * (vOffset.x+(vSize.x-1))/(vTotalSize.x-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.y)/(vTotalSize.y-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.z)/(vTotalSize.z-1) - bulbSize/2.0),              // 0

    DOUBLEVECTOR3(bulbSize * (vOffset.x)/(vTotalSize.x-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.y+(vSize.y-1))/(vTotalSize.y-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.z)/(vTotalSize.z-1) - bulbSize/2.0),              // 0

    DOUBLEVECTOR3(bulbSize * (vOffset.x+(vSize.x-1))/(vTotalSize.x-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.y+(vSize.y-1))/(vTotalSize.y-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.z)/(vTotalSize.z-1) - bulbSize/2.0),              // 0

    DOUBLEVECTOR3(bulbSize * (vOffset.x)/(vTotalSize.x-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.y)/(vTotalSize.y-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.z+(vSize.z-1))/(vTotalSize.z-1) - bulbSize/2.0),  // 1

    DOUBLEVECTOR3(bulbSize * (vOffset.x+(vSize.x-1))/(vTotalSize.x-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.y)/(vTotalSize.y-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.z+(vSize.z-1))/(vTotalSize.z-1) - bulbSize/2.0),  // 1

    DOUBLEVECTOR3(bulbSize * (vOffset.x)/(vTotalSize.x-1) - bulbSize/2.0,               // 0
                  bulbSize * (vOffset.y+(vSize.y-1))/(vTotalSize.y-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.z+(vSize.z-1))/(vTotalSize.z-1) - bulbSize/2.0),  // 1

    DOUBLEVECTOR3(bulbSize * (vOffset.x+(vSize.x-1))/(vTotalSize.x-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.y+(vSize.y-1))/(vTotalSize.y-1) - bulbSize/2.0,   // 1
                  bulbSize * (vOffset.z+(vSize.z-1))/(vTotalSize.z-1) - bulbSize/2.0),  // 1
  }};

  #pragma omp parallel for
  for (int i = 0;i<8;++i) {
    val[i] = (index != uint32_t(i)) ? ComputeMandelbulb<T>(pos[i].x,pos[i].y,pos[i].z, 8, iIterations, 100.0) : value;
  }


  if (vSize.x == 2 && vSize.y == 2 && vSize.z == 2) {
    std::array<T,2> l;
    UINT64VECTOR3 vPos;

    l[0] = val[0]; l[1] = val[1];
    vPos = vOffset;
    WriteLineAtPos<T>(pDummyData, 2, vPos, vTotalSize, l.data());
    l[0] = val[2]; l[1] = val[3];
    vPos = UINT64VECTOR3(vOffset.x, vOffset.y+1, vOffset.z);
    WriteLineAtPos<T>(pDummyData, 2, vPos, vTotalSize, l.data());
    l[0] = val[4]; l[1] = val[5];
    vPos = UINT64VECTOR3(vOffset.x, vOffset.y, vOffset.z+1);
    WriteLineAtPos<T>(pDummyData, 2, vPos, vTotalSize, l.data());
    l[0] = val[6]; l[1] = val[7];
    vPos = UINT64VECTOR3(vOffset.x, vOffset.y+1, vOffset.z+1);
    WriteLineAtPos<T>(pDummyData, 2, vPos, vTotalSize, l.data());
    return;
  }


  if (vSize.x > 4 && vSize.y > 4 && vSize.z > 4 && val[1] == val[0] && val[2] == val[0] && val[3] == val[0] && val[4] == val[0] &&
      val[5] == val[0] && val[6] == val[0] && val[7] == val[0] &&
      CheckBlockBoundary(val[0], iIterations, vOffset, vSize, vTotalSize) ) {
    //MESSAGE("Empty Brick @ %i, %i, %i of size %ix%ix%i\n", vOffset.x, vOffset.y, vOffset.z, vSize.x, vSize.y, vSize.z);
    FillBrick(pDummyData, val[0], vOffset, vSize, vTotalSize);
    return;
  }

  UINT64VECTOR3 vPos = vOffset;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 0, val[0], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.x += vSize.x/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 1, val[1], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.y += vSize.y/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 2, val[2], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.x += vSize.x/2; vPos.y += vSize.y/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 3, val[3], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.z += vSize.z/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 4, val[4], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.x += vSize.x/2; vPos.z += vSize.z/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 5, val[5], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.z += vSize.z/2; vPos.y += vSize.y/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 6, val[6], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  vPos = vOffset; vPos.x += vSize.x/2; vPos.y += vSize.y/2; vPos.z += vSize.z/2;
  ComputeFractalFast<T>(pDummyData, vPos, vSize/2, vTotalSize, timer, 7, val[7], depth+1, completed);
  completed += 1.0/pow(8.0, depth);

  if (vSize.volume() >= 16*16*16)
    MESSAGE(" %.3f%% completed (Depth=%i) (%s)", completed*100.0, depth, timer.GetProgressMessage(completed).c_str());


}

template<typename T, ECreationType eCreationType>
void GenerateVolumeData(UINT64VECTOR3 vSize, LargeRAWFile_ptr pDummyData,
                        uint32_t iIterations, bool bHierarchical) {

  if (iIterations == 0)
    iIterations = std::numeric_limits<T>::max()-1;
  ProgressTimer timer;
  timer.Start();

  // shortcut for fractals in an pow of two cube volume
  if (bHierarchical && eCreationType == CT_FRACTAL && vSize.x == vSize.y && vSize.y == vSize.z && vSize == vSize.makepow2()) {
    MESSAGE("Hierarchical Data Generation mode.");
    cout << endl;
	  ComputeFractalFast<T>(pDummyData, UINT64VECTOR3(0,0,0), vSize, vSize, timer);
	  return;
  }

  std::vector<T> source(vSize.x);

  for (uint64_t z = 0;z<vSize.z;z++) {
    const double completed = (double)z/vSize.z;
    MESSAGE("Generating Data %.3f%% completed (%s)",
            100.0*completed, timer.GetProgressMessage(completed).c_str());

    for (uint64_t y = 0;y<vSize.y;y++) {
      #pragma omp parallel for
      for (int64_t x = 0;x<int64_t(vSize.x);x++) {
        switch (eCreationType) {
          case CT_FRACTAL: {
            source[x] =
              ComputeMandelbulb(bulbSize * static_cast<double>(x)/
                                                     (vSize.x-1) - bulbSize/2.0,
                                               bulbSize * static_cast<double>(y)/
                                                     (vSize.y-1) - bulbSize/2.0,
                                               bulbSize * static_cast<double>(z)/
                                                     (vSize.z-1) - bulbSize/2.0,
                                               8,
                                               T(iIterations),
                                               100.0);
            }
            break;
          case CT_SPHERE:
            source[x] =
            static_cast<T>(std::max(0.0f,
                                   (0.5f-(0.5f-FLOATVECTOR3(float(x),
                                                            float(y),
                                                            float(z))/
                                              FLOATVECTOR3(vSize)).length())*
                                              std::numeric_limits<T>::max()*2));
            break;
          case CT_CONST_VALUE:
            source[x] = T(iIterations);
            break;
          case CT_RANDOM:
            source[x] = rand()%std::numeric_limits<T>::max();
            break;
        }
      }
      pDummyData->WriteRAW((uint8_t*)(source.data()), vSize.x*sizeof(T));
    }
  }
}

bool CreateUVFFile(const std::wstring& strUVFName, const UINT64VECTOR3& vSize,
                   uint32_t iBitSize, ECreationType eCreationType, uint32_t iIterations,
                   bool bUseToCBlock, bool bKeepRaw, uint32_t iCompression,
                   uint32_t iUVFMemory, uint32_t iBrickSize, uint32_t iLayout,
                   uint32_t iCompressionLevel, bool bHierarchical) {
  UVF uvfFile(strUVFName);

  const bool bGenerateUVF =
        SysTools::ToLowerCase(SysTools::GetExt(strUVFName)) == L"uvf";
  std::wstring rawFilename =
        bGenerateUVF ? SysTools::ChangeExt(strUVFName,L"raw") : strUVFName;

  MESSAGE("Generating dummy data");

  LargeRAWFile_ptr dummyData = LargeRAWFile_ptr(new LargeRAWFile(rawFilename));
  if (!dummyData->Create(vSize.volume()*iBitSize/8)) {
    T_ERROR("Failed to create %s file.", SysTools::toNarrow(rawFilename).c_str());
    return false;
  }

  Timer generationTimer;
  generationTimer.Start();
  switch (iBitSize) {
    case 8 :
      switch (eCreationType) {
        case CT_FRACTAL : MESSAGE("Generating a fractal"); GenerateVolumeData<uint8_t, CT_FRACTAL>(vSize, dummyData, iIterations, bHierarchical); break;
        case CT_SPHERE : MESSAGE("Generating a sphere"); GenerateVolumeData<uint8_t, CT_SPHERE>(vSize, dummyData, iIterations, bHierarchical); break;
        case CT_CONST_VALUE : MESSAGE("Generating zeroes"); GenerateVolumeData<uint8_t, CT_CONST_VALUE>(vSize, dummyData, 0, bHierarchical); break;
        case CT_RANDOM : MESSAGE("Generating noise"); GenerateVolumeData<uint8_t, CT_RANDOM>(vSize, dummyData, iIterations, bHierarchical); break;
      }
      break;
    case 16 :
      switch (eCreationType) {
        case CT_FRACTAL : MESSAGE("Generating a fractal"); GenerateVolumeData<uint16_t, CT_FRACTAL>(vSize, dummyData, iIterations, bHierarchical); break;
        case CT_SPHERE : MESSAGE("Generating a sphere"); GenerateVolumeData<uint16_t, CT_SPHERE>(vSize, dummyData, iIterations, bHierarchical); break;
        case CT_CONST_VALUE : MESSAGE("Generating zeroes"); GenerateVolumeData<uint16_t, CT_CONST_VALUE>(vSize, dummyData, 0, bHierarchical); break;
        case CT_RANDOM : MESSAGE("Generating noise"); GenerateVolumeData<uint16_t, CT_RANDOM>(vSize, dummyData, iIterations, bHierarchical); break;
      }
      break;
    default:
      T_ERROR("Invalid bitsize");
      return false;
  }
  dummyData->Close();

  uint64_t genMiliSecs = uint64_t(generationTimer.Elapsed());

  if (!bGenerateUVF) return EXIT_FAILURE;

  Timer uvfTimer;
  uvfTimer.Start();

  MESSAGE("Preparing creation of UVF file %s", SysTools::toNarrow(strUVFName).c_str());

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
  std::shared_ptr<TOCBlock> tocBlock(new TOCBlock(UVF::ms_ulReaderVersion));

  if (bUseToCBlock)  {
    MESSAGE("Buidling hirarchy ...");
    tocBlock->strBlockID = "Test TOC Volume 1";
    tocBlock->ulCompressionScheme = UVFTables::COS_NONE;

    bool bResult = tocBlock->FlatDataToBrickedLOD(rawFilename,
      L"./tempFile.tmp", iBitSize == 8 ? ExtendedOctree::CT_UINT8
                                      : ExtendedOctree::CT_UINT16,
      1, vSize, DOUBLEVECTOR3(1,1,1),
      UINT64VECTOR3(iBrickSize,iBrickSize,iBrickSize),
      DEFAULT_BRICKOVERLAP, false, false,
      1024*1024*1024*iUVFMemory, MaxMinData,
      &tuvok::Controller::Debug::Out(),
      static_cast<COMPRESSION_TYPE>(iCompression),
      iCompressionLevel,
      static_cast<LAYOUT_TYPE>(iLayout)
    );

    if (!bResult) {
      T_ERROR("Failed to subdivide the volume into bricks");
      dummyData->Delete();
      uvfFile.Close();
      return false;
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
                  if (!testRasterVolume->FlatDataToBrickedLOD(dummyData,
                    "./tempFile.tmp", CombineAverage<unsigned char,1>,
                    SimpleMaxMin<unsigned char,1>, MaxMinData,
                    &tuvok::Controller::Debug::Out())){
                    T_ERROR("Failed to subdivide the volume into bricks");
                    uvfFile.Close();
                    dummyData->Delete();
                    return false;
                  }
                  break;
                }
    case 16 :{
                if (!testRasterVolume->FlatDataToBrickedLOD(dummyData,
                  "./tempFile.tmp", CombineAverage<unsigned short,1>,
                  SimpleMaxMin<unsigned short,1>, MaxMinData,
                  &tuvok::Controller::Debug::Out())){
                  T_ERROR("Failed to subdivide the volume into bricks");
                  uvfFile.Close();
                  dummyData->Delete();
                  return false;
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
      return false;
    }

    pTestVolume = testRasterVolume;
  }

  if (!bKeepRaw) dummyData->Delete();

  if (!uvfFile.AddDataBlock(pTestVolume)) {
    T_ERROR("AddDataBlock failed!");
    uvfFile.Close();
    return false;
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
      return false;
    }
    Histogram1D->Compress(4096);
    MESSAGE("Computing 2D Histogram...");
    if (!Histogram2D->Compute(tocBlock.get(), 0,
                              Histogram1D->GetHistogram().size(),
                              MaxMinData->GetGlobalValue().maxScalar)) {
      T_ERROR("Computation of 2D Histogram failed!");
      uvfFile.Close();
      return false;
    }
  } else {
    if (!Histogram1D->Compute(testRasterVolume.get())) {
      T_ERROR("Computation of 1D Histogram failed!");
      uvfFile.Close();
      return false;
    }
    Histogram1D->Compress(4096);
    MESSAGE("Computing 2D Histogram...");
    if (!Histogram2D->Compute(testRasterVolume.get(),
                              Histogram1D->GetHistogram().size(),
                              MaxMinData->GetGlobalValue().maxScalar)) {
      T_ERROR("Computation of 2D Histogram failed!");
      uvfFile.Close();
      return false;
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
  metaPairs->AddPair(L"Data Source",L"This file was created by the UVFReader");
  metaPairs->AddPair(L"Description",L"Dummy file for testing purposes.");

  if (EndianConvert::IsLittleEndian())
    metaPairs->AddPair(L"Source Endianess",L"little");
  else
    metaPairs->AddPair(L"Source Endianess",L"big");

  metaPairs->AddPair(L"Source Type",L"integer");
  metaPairs->AddPair(L"Source Bit width",SysTools::ToWString(iBitSize));

  uvfFile.AddDataBlock(metaPairs);

  MESSAGE("Writing UVF file...");

  if (!uvfFile.Create()) {
    T_ERROR("Failed to create UVF file %s", SysTools::toNarrow(strUVFName).c_str());
    return false;
  }

  MESSAGE("Computing checksum...");
  uvfFile.Close();

  uint64_t uvfMiliSecs = uint64_t(uvfTimer.Elapsed());
  const uint64_t uvfSecs  = (uvfMiliSecs/1000)%60;
  const uint64_t uvfMins  = (uvfMiliSecs/60000)%60;
  const uint64_t uvfHours = (uvfMiliSecs/3600000);

  const uint64_t genSecs  = (genMiliSecs/1000)%60;
  const uint64_t genMins  = (genMiliSecs/60000)%60;
  const uint64_t genHours = (genMiliSecs/3600000);

  MESSAGE("Successfully created UVF file %s (generator time: %i:%02i:%02i  "
                                            "UVF time: %i:%02i:%02i)",
    SysTools::toNarrow(strUVFName).c_str(), int(genHours), int(genMins), int(genSecs),
          int(uvfHours), int(uvfMins), int(uvfSecs));
  return true;
}

#endif // DATASOURCE_H

/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Interactive Visualization and Data Analysis Group

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
