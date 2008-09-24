#pragma once

#ifndef HISTOGRAM2DDATABLOCK_H
#define HISTOGRAM2DDATABLOCK_H

#include "RasterDataBlock.h"

#include <string>

class Histogram2DDataBlock : public RasterDataBlock {
public:
  Histogram2DDataBlock();
  Histogram2DDataBlock(const Histogram2DDataBlock &other);
  Histogram2DDataBlock(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);
  virtual ~Histogram2DDataBlock();
	virtual DataBlock* Clone();

  bool Compute(RasterDataBlock* source);
  virtual UINT64 CopyToFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian, bool bIsLastBlock);

  const std::vector< std::vector<UINT64> >& GetHistogram() {return m_vHistData;}
  virtual bool SetBlockSemantic(UVFTables::BlockSemanticTable) {return false;}

protected:
  virtual UINT64 GetHeaderFromFile(LargeRAWFile* pStreamFile, UINT64 iOffset, bool bIsBigEndian);

private:
  std::vector< std::vector<UINT64> > m_vHistData;

};

#endif // HISTOGRAM2DDATABLOCK_H
