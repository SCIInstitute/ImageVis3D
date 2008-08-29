#pragma once

#ifndef RASTERDATABLOCK_H
#define RASTERDATABLOCK_H

#include "DataBlock.h"

#include <string>

/*
template<class T> void CombineAverage(std::vector<UINT64> vSourcePos, UINT64 iTargetPos, LargeRAWFile* pDataIn, LargeRAWFile* pDataOut) {
	double temp = 0;
  T TValue;
	for (size_t i = 0;i<vSourcePos.size();i++) {
    pDataIn->Read<T>(&TValue,1,vSourcePos[i],0);
		temp += double(TValue);
	}
	// make sure not to touch pDataOut before we are finished with reading pDataIn, this allows for inplace combine calls
  TValue = T(temp/double(vSourcePos.size()));
	pDataOut->Write(&TValue, 1, iTargetPos, 0);
}

template<class T, UINT64 iVecLength> void CombineAverage(std::vector<UINT64> vSourcePos, UINT64 iTargetPos, LargeRAWFile* pDataIn, LargeRAWFile* pDataOut) {

	double temp[iVecLength];	for (UINT64 v = 0;v<iVecLength;v++) temp[v] = 0;

  T TValue[iVecLength];
	for (UINT64 i = 0;i<vSourcePos.size();i++) {
    pDataIn->Read(TValue,iVecLength,vSourcePos[i]*iVecLength,0);
		for (UINT64 v = 0;v<iVecLength;v++) temp[v] += double(TValue[v]) / double(vSourcePos.size());
	}
	// make sure not to touch pDataOut before we are finished with reading pDataIn, this allows for inplace combine calls
	for (UINT64 v = 0;v<iVecLength;v++) TValue = T(temp[v]);
	pDataOut->Write(TValue, iVecLength, iTargetPos, 0);
}
*/

template<class T> void CombineAverage(std::vector<UINT64> vSource, UINT64 iTarget, const void* pIn, const void* pOut) {
	const T *pDataIn = (T*)pIn;
	T *pDataOut = (T*)pOut;

	double temp = 0;
	for (size_t i = 0;i<vSource.size();i++) {
		temp += double(pDataIn[vSource[i]]);
	}
	// make sure not to touch pDataOut before we are finished with reading pDataIn, this allows for inplace combine calls
	pDataOut[iTarget] = T(temp / double(vSource.size()));
}

template<class T, UINT64 iVecLength> void CombineAverage(std::vector<UINT64> vSource, UINT64 iTarget, const void* pIn, const void* pOut) {
	const T *pDataIn = (T*)pIn;
	T *pDataOut = (T*)pOut;

	double temp[iVecLength];	for (UINT64 v = 0;v<iVecLength;v++) temp[v] = 0;

	for (size_t i = 0;i<vSource.size();i++) {
		for (UINT64 v = 0;v<iVecLength;v++) temp[size_t(v)] += double(pDataIn[v+vSource[i]*iVecLength]) / double(vSource.size());
	}
	// make sure not to touch pDataOut before we are finished with reading pDataIn, this allows for inplace combine calls
	for (UINT64 v = 0;v<iVecLength;v++) 
		pDataOut[v+iTarget*iVecLength] = T(temp[v]);
}

class RasterDataBlock : public DataBlock {
public:
	RasterDataBlock();
	RasterDataBlock(const RasterDataBlock &other);
	RasterDataBlock(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);
	virtual RasterDataBlock& operator=(const RasterDataBlock& other);
	virtual ~RasterDataBlock();

	virtual bool Verify(UINT64 iSizeofData, std::string* pstrProblem = NULL) const;
	virtual bool Verify(std::string* pstrProblem = NULL) const;

	std::vector<UVFTables::DomainSemanticTable> ulDomainSemantics;
	std::vector<double> dDomainTransformation;
	std::vector<UINT64> ulDomainSize;
	std::vector<UINT64> ulBrickSize;
	std::vector<UINT64> ulBrickOverlap;
	std::vector<UINT64> ulLODDecFactor;
	std::vector<UINT64> ulLODGroups;
	std::vector<UINT64> ulLODLevelCount;
	UINT64 ulElementDimension;
	std::vector<UINT64> ulElementDimensionSize;
	std::vector<std::vector<UVFTables::ElementSemanticTable> > ulElementSemantic;
	std::vector<std::vector<UINT64> > ulElementBitSize;
	std::vector<std::vector<UINT64> > ulElementMantissa;
	std::vector<std::vector<bool> > bSignedElement;
	UINT64 ulOffsetToDataBlock;

	virtual UINT64 ComputeDataSize(std::string* pstrProblem = NULL) const ;
	virtual UINT64 ComputeHeaderSize() const;

	bool SetBlockSemantic(UVFTables::BlockSemanticTable bs);

	// CONVENIANCE FUNCTIONS
	void SetScaleOnlyTransformation(const std::vector<double>& vScale);
	void SetIdentityTransformation();
	void SetTypeToScalar(UINT64 iBitWith, UINT64 iMantissa, bool bSigned, UVFTables::ElementSemanticTable semantic);
	void SetTypeToVector(UINT64 iBitWith, UINT64 iMantissa, bool bSigned, std::vector<UVFTables::ElementSemanticTable> semantic);
	void SetTypeToUByte(UVFTables::ElementSemanticTable semantic);
	void SetTypeToUShort(UVFTables::ElementSemanticTable semantic);
	void SetTypeToFloat(UVFTables::ElementSemanticTable semantic);
	void SetTypeToDouble(UVFTables::ElementSemanticTable semantic);

	void GetDataPointer(unsigned char** vData, const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) const;

	const std::vector<UINT64>& GetBrickCount(const std::vector<UINT64>& vLOD) const {return m_vBrickCount[size_t(PermutationToIndex(vLOD, ulLODLevelCount))];}
	const std::vector<UINT64>& GetBrickSize(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) const {UINT64 iLODIndex = PermutationToIndex(vLOD, ulLODLevelCount);return m_vBrickSizes[size_t(iLODIndex)][size_t(PermutationToIndex(vBrick, m_vBrickCount[size_t(iLODIndex)]))];}

	void FlatDataToBrickedLOD(const void* pSourceData, const std::string& strTempFile = "tempFile.tmp", void (*combineFunc)(std::vector<UINT64> vSource, UINT64 iTarget, const void* pIn, const void* pOut) = CombineAverage<char>);
  void FlatDataToBrickedLOD(LargeRAWFile* pSourceData, const std::string& strTempFile = "tempFile.tmp", void (*combineFunc)(std::vector<UINT64> vSource, UINT64 iTarget, const void* pIn, const void* pOut) = CombineAverage<char>);
	void AllocateTemp(const std::string& strTempFile, bool bBuildOffsetTables=false);

protected:
	LargeRAWFile* m_pTempFile;

	virtual UINT64 GetHeaderFromFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);
	virtual UINT64 CopyToFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian, bool bIsLastBlock);
	virtual DataBlock* Clone();
	virtual UINT64 GetOffsetToNextBlock() const;

	std::vector<std::vector<UINT64> > CountToVectors(std::vector<UINT64> vCountVector) const ;
	UINT64 ComputeElementSize() const;
	virtual UINT64 GetLODSize(std::vector<UINT64>& vLODIndices) const;
	virtual UINT64 ComputeLODLevelSize(const std::vector<UINT64>& vReducedDomainSize) const;
	virtual std::vector<std::vector<UINT64> > ComputeBricks(const std::vector<UINT64>& vDomainSize) const;
	virtual std::vector<std::vector<UINT64> > GenerateAllPermutations(const std::vector<std::vector<UINT64> >& vElements, UINT64 iIndex=0) const;
	UINT64 RecompLODIndexCount() const;
	void CleanupTemp();


	friend class UVF;

	// CONVENIANCE FUNCTION HELPERS
	std::vector<UINT64> m_vLODOffsets;
	std::vector<std::vector<UINT64> > m_vBrickCount;
	std::vector<std::vector<UINT64> > m_vBrickOffsets;
	std::vector<std::vector<std::vector<UINT64> > > m_vBrickSizes;

	UINT64 PermutationToIndex(const std::vector<UINT64>& vec, const std::vector<UINT64>& vSizes) const;
	UINT64 GetLocalDataPointerOffset(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) const;
	UINT64 GetLocalDataPointerOffset(const UINT64 iLODIndex, const UINT64 iBrickIndex) const {return m_vLODOffsets[size_t(iLODIndex)] + m_vBrickOffsets[size_t(iLODIndex)][size_t(iBrickIndex)];}
	void SubSample(LargeRAWFile* pSourceFile, LargeRAWFile* pTargetFile, std::vector<UINT64> sourceSize, std::vector<UINT64> targetSize, void (*combineFunc)(std::vector<UINT64> vSource, UINT64 iTarget, const void* pIn, const void* pOut));
	UINT64 ComputeDataSizeAndOffsetTables();
	UINT64 GetLODSizeAndOffsetTables(std::vector<UINT64>& vLODIndices, UINT64 iLOD);
	UINT64 ComputeLODLevelSizeAndOffsetTables(const std::vector<UINT64>& vReducedDomainSize, UINT64 iLOD);
};

#endif // RASTERDATABLOCK_H
