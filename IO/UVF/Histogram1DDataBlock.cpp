#include "Histogram1DDataBlock.h"

#include <memory.h>
using namespace std;


Histogram1DDataBlock::Histogram1DDataBlock() : RasterDataBlock() {
  ulBlockSemantics = UVFTables::BS_1D_Histogram;
}

Histogram1DDataBlock::Histogram1DDataBlock(const Histogram1DDataBlock &other) :
  RasterDataBlock(other),
  m_vHistData(other.m_vHistData)
{
}

Histogram1DDataBlock::Histogram1DDataBlock(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian) :
  RasterDataBlock(pStreamFile, iOffset, bIsBigEndian) 
{
  m_vHistData.resize(ulDomainSize[0]);
  pStreamFile->ReadRAW((unsigned char*)&m_vHistData[0], ulDomainSize[0]*sizeof(UINT64));

  ulBlockSemantics = UVFTables::BS_1D_Histogram;
}

Histogram1DDataBlock::~Histogram1DDataBlock() 
{
}

DataBlock* Histogram1DDataBlock::Clone() {
	return new Histogram1DDataBlock(*this);
}

UINT64 Histogram1DDataBlock::GetHeaderFromFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian) {
  iOffset = RasterDataBlock::GetHeaderFromFile(pStreamFile, iOffset, bIsBigEndian);
  m_vHistData.resize(ulDomainSize[0]);
  pStreamFile->ReadRAW((unsigned char*)&m_vHistData[0], ulDomainSize[0]*sizeof(UINT64));
  return iOffset;
}

bool Histogram1DDataBlock::Compute(RasterDataBlock* source) {

  // TODO: right now we can only compute Histograms of scalar data this should be changed to a more general approach
  if (source->ulElementDimension != 1 || source->ulElementDimensionSize.size() != 1) return false;


  // TODO: right now compute Histogram assumes that the lowest LOD level consists only of a single brick, this brick is used for the hist. computation
  //       this should be changed to a more general approach
  vector<UINT64> vSmallestLOD = source->GetSmallestBrickIndex();
  const vector<UINT64>& vBricks = source->GetBrickCount(vSmallestLOD);
  for (unsigned int i = 0;i<vBricks.size();i++) if (vBricks[i] != 1) return false;
  
  // create temp histogram 
  UINT64 iValueRange = 1<<(source->ulElementBitSize[0][0]);
  vector<UINT64> vTmpHist(iValueRange);
  if (vTmpHist.size() != iValueRange) return false;
  for (UINT64 i = 0;i<iValueRange;i++) vTmpHist[i] = 0;

  // find smalest brick in hirarchy
  unsigned char *pcSourceData = NULL;
  vector<UINT64> vLOD = source->GetSmallestBrickIndex();
  vector<UINT64> vOneAndOnly;
  for (size_t i = 0;i<vBricks.size();i++) vOneAndOnly.push_back(0);
  if (!source->GetData(&pcSourceData, vLOD, vOneAndOnly)) return false;

  vector<UINT64> vSize  = source->GetSmallestBrickSize();

  UINT64 iDataSize = 1;
  for (size_t i = 0;i<vSize.size();i++) iDataSize*=vSize[i];

  // TODO: right now only 8 and 16 bit integer data is supported this should be changed to a more general approach
  if (source->ulElementBitSize[0][0] == 8) {
    for (UINT64 i = 0;i<iDataSize;i++) {
       vTmpHist[pcSourceData[i]]++;
    }
  } else {
    if (source->ulElementBitSize[0][0] == 16) {
      unsigned short *psSourceData = (unsigned short*)pcSourceData;
      for (UINT64 i = 0;i<iDataSize;i++) {
        vTmpHist[psSourceData[i]]++;
      }
    } else {
      delete [] pcSourceData;
      return false;
    }
  }
  delete [] pcSourceData;

  // find maximum non zero entry
  size_t iSize = 0;
  for (size_t i = 0;i<iValueRange;i++) if (vTmpHist[i] != 0) iSize = i+1;
  iValueRange = iSize;
  
  // copy non zero elements in temp histogram to histogram
  m_vHistData.resize(iValueRange);
  if (m_vHistData.size() != iValueRange) return false;
  for (UINT64 i = 0;i<iValueRange;i++) m_vHistData[i] = vTmpHist[i];

  // set raster data block information
  strBlockID = "1D Histogram for datablock " + source->strBlockID;
	ulCompressionScheme = UVFTables::COS_NONE;
	ulDomainSemantics.push_back(UVFTables::DS_X);
  ulDomainSize.push_back(iValueRange);
	ulLODDecFactor.push_back(2);
	ulLODGroups.push_back(0);
	ulLODLevelCount.push_back(1);
  SetTypeToUInt64(UVFTables::ES_UNKNOWN);
	ulBrickSize.push_back(iValueRange);
	ulBrickOverlap.push_back(1);
	SetIdentityTransformation();

  return true;
}

UINT64 Histogram1DDataBlock::CopyToFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian, bool bIsLastBlock) {
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
  pStreamFile->WriteRAW((unsigned char*)&m_vHistData[0], ulDomainSize[0]*sizeof(UINT64));

	return pStreamFile->GetPos() - iOffset;  
}