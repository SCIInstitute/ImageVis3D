#include "Histogram2DDataBlock.h"
#include <Tools/Vectors.h>

#include <memory.h>
using namespace std;


Histogram2DDataBlock::Histogram2DDataBlock() : RasterDataBlock() {
  ulBlockSemantics = UVFTables::BS_2D_Histogram;
}

Histogram2DDataBlock::Histogram2DDataBlock(const Histogram2DDataBlock &other) : 
  RasterDataBlock(other),
  m_vHistData(other.m_vHistData)
{
}

Histogram2DDataBlock::Histogram2DDataBlock(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian) :
  RasterDataBlock(pStreamFile, iOffset, bIsBigEndian) 
{
  m_vHistData.resize(ulDomainSize[0]);
  vector<UINT64> tmp(ulDomainSize[1]);
  for (UINT64 i = 0;i<ulDomainSize[0];i++) {
    pStreamFile->ReadRAW((unsigned char*)&tmp[0], ulDomainSize[1]*sizeof(UINT64));
    m_vHistData[i] = tmp;
  }

  ulBlockSemantics = UVFTables::BS_2D_Histogram;
}

Histogram2DDataBlock::~Histogram2DDataBlock() 
{
}

DataBlock* Histogram2DDataBlock::Clone() {
	return new Histogram2DDataBlock(*this);
}

UINT64 Histogram2DDataBlock::GetHeaderFromFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian) {
  iOffset = RasterDataBlock::GetHeaderFromFile(pStreamFile, iOffset, bIsBigEndian);
  m_vHistData.resize(ulDomainSize[0]);
  vector<UINT64> tmp(ulDomainSize[1]);
  for (UINT64 i = 0;i<ulDomainSize[0];i++) {
    pStreamFile->ReadRAW((unsigned char*)&tmp[0], ulDomainSize[1]*sizeof(UINT64));
    m_vHistData[i] = tmp;
  }
  return iOffset;
}

// TODO: right now compute Histogram assumes that the lowest LOD level consists only of a single brick, this brick is used for the hist. computation
//       this should be changed to a more general approach
bool Histogram2DDataBlock::Compute(RasterDataBlock* source) {
  // TODO: right now we can only compute Histograms of scalar data this should be changed to a more general approach
  if (source->ulElementDimension != 1 || source->ulElementDimensionSize.size() != 1) return false;

  // TODO: right now compute Histogram assumes that the lowest LOD level consists only of a single brick, this brick is used for the hist. computation
  //       this should be changed to a more general approach
  vector<UINT64> vSmallestLOD = source->GetSmallestBrickIndex();
  const vector<UINT64>& vBricks = source->GetBrickCount(vSmallestLOD);
  for (unsigned int i = 0;i<vBricks.size();i++) if (vBricks[i] != 1) return false;
  
  // TODO: right now we can only compute 2D Histograms of at least 3D data this should be changed to a more general approach
  //       also we require that the first three entries as X,Y,Z
  if (source->ulDomainSize.size() < 3 || source->ulDomainSemantics[0] != UVFTables::DS_X ||
      source->ulDomainSemantics[1] != UVFTables::DS_Y || source->ulDomainSemantics[2] != UVFTables::DS_Z) return false;

  UINT64 iValueRange = 1<<(source->ulElementBitSize[0][0]);

  std::vector< std::vector<UINT64> > vTmpHist(iValueRange);
  for (UINT64 i = 0;i<iValueRange;i++) {
    vTmpHist[i].resize(256);
    for (UINT64 j = 0;j<256;j++) {
      vTmpHist[i][j] = 0;
    }
  }

  unsigned char *pcSourceData = NULL;

  vector<UINT64> vLOD = source->GetSmallestBrickIndex();
  vector<UINT64> vOneAndOnly;
  for (size_t i = 0;i<vBricks.size();i++) vOneAndOnly.push_back(0);
  if (!source->GetData(&pcSourceData, vLOD, vOneAndOnly)) return false;

  vector<UINT64> vSize  = source->GetSmallestBrickSize();

  UINT64 iDataSize = 1;
  for (size_t i = 0;i<vSize.size();i++) iDataSize*=vSize[i];

  // TODO: right now only 8 and 16 bit integer data is supported this should be changed to a more general approach
  float fMaxGrad = 0.0f;
  if (source->ulElementBitSize[0][0] == 8) {
    for (UINT64 z = 1;z<vSize[2]-1;z++) {
      for (UINT64 y = 1;y<vSize[1]-1;y++) {
        for (UINT64 x = 1;x<vSize[0]-1;x++) {

          size_t iCenter = x+vSize[0]*y+vSize[0]*vSize[1]*z;
          size_t iLeft   = iCenter-1;
          size_t iRight  = iCenter+1;
          size_t iTop    = iCenter-vSize[0];
          size_t iBottom = iCenter+vSize[0];
          size_t iFront  = iCenter-vSize[0]*vSize[1];
          size_t iBack   = iCenter+vSize[0]*vSize[1];

          FLOATVECTOR3   vGradient(float(pcSourceData[iLeft]-pcSourceData[iRight]),
                                   float(pcSourceData[iTop]-pcSourceData[iBottom]),
                                   float(pcSourceData[iFront]-pcSourceData[iBack]));

          if (vGradient.length() > fMaxGrad) fMaxGrad = vGradient.length();
        }
      }
    }
    for (UINT64 z = 1;z<vSize[2]-1;z++) {
      for (UINT64 y = 1;y<vSize[1]-1;y++) {
        for (UINT64 x = 1;x<vSize[0]-1;x++) {

          size_t iCenter = x+vSize[0]*y+vSize[0]*vSize[1]*z;
          size_t iLeft   = iCenter-1;
          size_t iRight  = iCenter+1;
          size_t iTop    = iCenter-vSize[0];
          size_t iBottom = iCenter+vSize[0];
          size_t iFront  = iCenter-vSize[0]*vSize[1];
          size_t iBack   = iCenter+vSize[0]*vSize[1];

          FLOATVECTOR3   vGradient(float(pcSourceData[iLeft]-pcSourceData[iRight]),
                                   float(pcSourceData[iTop]-pcSourceData[iBottom]),
                                   float(pcSourceData[iFront]-pcSourceData[iBack]));

          unsigned char iGardientMagnitudeIndex = (unsigned char)(vGradient.length()/fMaxGrad*255);
          vTmpHist[pcSourceData[iCenter]][iGardientMagnitudeIndex]++;
        }
      }
    }
  } else {
    if (source->ulElementBitSize[0][0] == 16) {
      unsigned short *psSourceData = (unsigned short*)pcSourceData;
      for (UINT64 z = 1;z<vSize[2]-1;z++) {
        for (UINT64 y = 1;y<vSize[1]-1;y++) {
          for (UINT64 x = 1;x<vSize[0]-1;x++) {

            size_t iCenter = x+vSize[0]*y+vSize[0]*vSize[1]*z;
            size_t iLeft   = iCenter-1;
            size_t iRight  = iCenter+1;
            size_t iTop    = iCenter-vSize[0];
            size_t iBottom = iCenter+vSize[0];
            size_t iFront  = iCenter-vSize[0]*vSize[1];
            size_t iBack   = iCenter+vSize[0]*vSize[1];

            FLOATVECTOR3   vGradient(float(psSourceData[iLeft]-psSourceData[iRight]),
                                     float(psSourceData[iTop]-psSourceData[iBottom]),
                                     float(psSourceData[iFront]-psSourceData[iBack]));

            if (vGradient.length() > fMaxGrad) fMaxGrad = vGradient.length();
          }
        }
      }
      for (UINT64 z = 1;z<vSize[2]-1;z++) {
        for (UINT64 y = 1;y<vSize[1]-1;y++) {
          for (UINT64 x = 1;x<vSize[0]-1;x++) {

            size_t iCenter = x+vSize[0]*y+vSize[0]*vSize[1]*z;
            size_t iLeft   = iCenter-1;
            size_t iRight  = iCenter+1;
            size_t iTop    = iCenter-vSize[0];
            size_t iBottom = iCenter+vSize[0];
            size_t iFront  = iCenter-vSize[0]*vSize[1];
            size_t iBack   = iCenter+vSize[0]*vSize[1];

            FLOATVECTOR3   vGradient(float(psSourceData[iLeft]-psSourceData[iRight]),
                                     float(psSourceData[iTop]-psSourceData[iBottom]),
                                     float(psSourceData[iFront]-psSourceData[iBack]));

            unsigned char iGardientMagnitudeIndex = (unsigned char)(vGradient.length()/fMaxGrad*255);
            vTmpHist[psSourceData[iCenter]][iGardientMagnitudeIndex]++;
          }
        }
      }
    } else {
      delete [] pcSourceData;
      return false;
    }
  }

  delete [] pcSourceData;

  size_t iSize = 0;
  for (size_t x = iSize;x<iValueRange;x++) {
    for (size_t y = 0;y<256;y++) {
      if (vTmpHist[x][y] != 0) iSize = x+1;
    }
  }
  iValueRange = iSize;


  m_vHistData.resize(iValueRange);
  for (UINT64 i = 0;i<iValueRange;i++) {
    m_vHistData[i].resize(256);
    for (UINT64 j = 0;j<256;j++) {
      m_vHistData[i][j] = vTmpHist[i][j];
    }
  }


  // set raster data block information
	strBlockID = "2D Histogram for datablock " + source->strBlockID;
	ulCompressionScheme = UVFTables::COS_NONE;
	ulDomainSemantics.push_back(UVFTables::DS_X);
	ulDomainSemantics.push_back(UVFTables::DS_Y);
  ulDomainSize.push_back(m_vHistData.size());
  ulDomainSize.push_back(m_vHistData[0].size());
	ulLODDecFactor.push_back(2);
	ulLODDecFactor.push_back(2);
	ulLODGroups.push_back(0);
  ulLODGroups.push_back(0);
	ulLODLevelCount.push_back(1);
  SetTypeToUInt64(UVFTables::ES_UNKNOWN);
	ulBrickSize.push_back(m_vHistData.size());
  ulBrickSize.push_back(m_vHistData[0].size());
	ulBrickOverlap.push_back(1);
  ulBrickOverlap.push_back(1);
	SetIdentityTransformation();


  return true;
}

UINT64 Histogram2DDataBlock::CopyToFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian, bool bIsLastBlock) {
  UINT64 iStart = iOffset + DataBlock::CopyToFile(pStreamFile, iOffset, bIsBigEndian, bIsLastBlock);
	pStreamFile->SeekPos(iStart);

	// write header
	UINT64 ulDomainDimension = ulDomainSemantics.size();
	pStreamFile->WriteData(ulDomainDimension, bIsBigEndian);

	if (ulDomainDimension > 0) {
		vector<UINT64> uintVect; uintVect.resize(size_t(ulDomainDimension));
		for (size_t i = 0;i<uintVect.size();i++) uintVect[i] = (UINT64)ulDomainSemantics[i];
		pStreamFile->WriteData(uintVect, bIsBigEndian);

		pStreamFile->WriteData(dDomainTransformation, bIsBigEndian);
		pStreamFile->WriteData(ulDomainSize, bIsBigEndian);
		pStreamFile->WriteData(ulBrickSize, bIsBigEndian);
		pStreamFile->WriteData(ulBrickOverlap, bIsBigEndian);
		pStreamFile->WriteData(ulLODDecFactor, bIsBigEndian);
		pStreamFile->WriteData(ulLODGroups, bIsBigEndian);
	}

	pStreamFile->WriteData(ulLODLevelCount, bIsBigEndian);
	pStreamFile->WriteData(ulElementDimension, bIsBigEndian);
	pStreamFile->WriteData(ulElementDimensionSize, bIsBigEndian);

	for (size_t i = 0;i<size_t(ulElementDimension);i++) {

		vector<UINT64> uintVect; uintVect.resize(size_t(ulElementDimensionSize[i]));
		for (size_t j = 0;j<uintVect.size();j++)  uintVect[j] = (UINT64)ulElementSemantic[i][j];
		pStreamFile->WriteData(uintVect, bIsBigEndian);				

		pStreamFile->WriteData(ulElementBitSize[i], bIsBigEndian);
		pStreamFile->WriteData(ulElementMantissa[i], bIsBigEndian);

		// writing bools failed on windows so we are writing chars
		vector<char> charVect; charVect.resize(size_t(ulElementDimensionSize[i]));
		for (size_t j = 0;j<charVect.size();j++)  charVect[j] = bSignedElement[i][j] ? 1 : 0;
		pStreamFile->WriteData(charVect, bIsBigEndian);				
	}

	pStreamFile->WriteData(ulOffsetToDataBlock, bIsBigEndian);

	UINT64 iDataSize = ComputeDataSize();

  pStreamFile->SeekPos( pStreamFile->GetPos() + ulOffsetToDataBlock);
  vector<UINT64> tmp;
  for (size_t i = 0;i<ulDomainSize[0];i++) {
    tmp = m_vHistData[i];
    pStreamFile->WriteRAW((unsigned char*)&tmp[0], ulDomainSize[1]*sizeof(UINT64));
  }

	return pStreamFile->GetPos() - iOffset;  
}