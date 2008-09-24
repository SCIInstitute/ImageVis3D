#pragma once

#ifndef HISTOGRAM1DDATABLOCK_H
#define HISTOGRAM1DDATABLOCK_H

#include "RasterDataBlock.h"

#include <string>

class Histogram1DDataBlock : public RasterDataBlock {
public:
  Histogram1DDataBlock();
  Histogram1DDataBlock(const Histogram1DDataBlock &other);
  Histogram1DDataBlock(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);
  virtual ~Histogram1DDataBlock();
	virtual DataBlock* Clone();

  bool Compute(RasterDataBlock* source);
  virtual UINT64 CopyToFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian, bool bIsLastBlock);

  const std::vector<UINT64>& GetHistogram() {return m_vHistData;}
  virtual bool SetBlockSemantic(UVFTables::BlockSemanticTable) {return false;}

protected:
  virtual UINT64 GetHeaderFromFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);

private:
  std::vector<UINT64> m_vHistData;

};

#endif // HISTOGRAM1DDATABLOCK_H
