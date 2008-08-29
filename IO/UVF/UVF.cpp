#include "UVF.h"
#include <Basics/SysTools.h>
#include <Basics/Checksums/crc32.h>
#include <Basics/Checksums/MD5.h>
#include <sstream>


using namespace std;
using namespace UVFTables;


UINT64 UVF::ms_ulReaderVersion = UVFVERSION;


UVF::UVF(std::wstring wstrFilename) : 
	m_bFileIsLoaded(false),
	m_bFileIsReadWrite(false),
  m_streamFile(wstrFilename)
{
}

UVF::~UVF(void)
{
	Close();
}

bool UVF::Open(bool bVerify, bool bReadWrite, std::string* pstrProblem) {
	if (m_bFileIsLoaded) return true;

  m_bFileIsLoaded = m_streamFile.Open(bReadWrite);

  if (!m_bFileIsLoaded) {
    (*pstrProblem) = "file not found or access denied";
    return false;
  }
	m_bFileIsReadWrite = bReadWrite;

	if (ParseGlobalHeader(bVerify,pstrProblem)) {
		ParseDataBlocks();		
		return true;
	} else {
		Close(); // file is not a UVF file or checksum is invalid
		return false;
	}
}

void UVF::Close() {
  if (m_bFileIsLoaded) {
  	if (m_bFileIsReadWrite) UpdateChecksum();
    m_streamFile.Close();
  	m_bFileIsLoaded = false;
  }

	for (size_t i = 0;i<m_DataBlocks.size();i++) delete m_DataBlocks[i];
	m_DataBlocks.resize(0);
}


bool UVF::ParseGlobalHeader(bool bVerify, std::string* pstrProblem) {
  if (m_streamFile.GetCurrentSize() < GlobalHeader::GetMinSize() + 8) {
    (*pstrProblem) = "file to small to be a UVF file";
    return false;
  }

  unsigned char pData[8];
  m_streamFile.ReadRAW(pData, 8);

  if (pData[0] != 'U' || pData[1] != 'V' || pData[2] != 'F' || pData[3] != '-' || pData[4] != 'D' || pData[5] != 'A' || pData[6] != 'T' || pData[7] != 'A') {
    (*pstrProblem) = "file magic not found";
    return false;
  }
	
	m_GlobalHeader.GetHeaderFromFile(&m_streamFile);
	
	return !bVerify || VerifyChecksum(pstrProblem);
}


vector<unsigned char> UVF::ComputeChecksum(ChecksumSemanticTable eChecksumSemanticsEntry) {
	vector<unsigned char> checkSum;

	UINT64 iOffset    = 33+UVFTables::ChecksumElemLength(eChecksumSemanticsEntry);
  UINT64 iFileSize  = m_streamFile.GetCurrentSize();
	UINT64 iSize      = iFileSize-iOffset;

  m_streamFile.SeekPos(iOffset);

  unsigned char *ucBlock=new unsigned char[1<<20];
	switch (eChecksumSemanticsEntry) {
		case CS_CRC32 : {
							CRC32		 crc;
              UINT64 iBlocks=iFileSize>>20;
	            unsigned long dwCRC32=0xFFFFFFFF;
	            for (UINT64 i=0; i<iBlocks; i++) {
		            m_streamFile.ReadRAW(ucBlock,1<<20);
		            crc.chunk(ucBlock,1<<20,dwCRC32);
	            }

	            size_t iLengthLastChunk=size_t(iFileSize-(iBlocks<<20));
              m_streamFile.ReadRAW(ucBlock,iLengthLastChunk);
	            crc.chunk(ucBlock,iLengthLastChunk,dwCRC32);
	            dwCRC32^=0xFFFFFFFF;

							for (UINT64 i = 0;i<4;i++) {
								unsigned char c = dwCRC32 & 255;
								checkSum.push_back(c);
								dwCRC32 = dwCRC32>>8;
							}
						} break;
		case CS_MD5 : {
							MD5		md5;
							int		iError=0;
							UINT	iBlockSize;

							while (iSize > 0)
							{
								iBlockSize = UINT(min(iSize,UINT64(1<<20)));

                m_streamFile.ReadRAW(ucBlock,iBlockSize);
								md5.Update(ucBlock, iBlockSize, iError);
								iSize   -= iBlockSize;
							}

							checkSum = md5.Final(iError);

						} break;
		case CS_NONE : 
		default		 : break;
	}
  delete [] ucBlock;

  m_streamFile.SeekStart();
	return checkSum;
}


bool UVF::VerifyChecksum(std::string* pstrProblem) {
	vector<unsigned char> vecActualCheckSum = ComputeChecksum(m_GlobalHeader.ulChecksumSemanticsEntry);

	if (vecActualCheckSum.size() != m_GlobalHeader.vcChecksum.size()) {
		if (pstrProblem != NULL)  {
			stringstream s;
			string strActual = "", strFile = "";
			for (size_t i = 0;i<vecActualCheckSum.size();i++) strActual += vecActualCheckSum[i];
			for (size_t i = 0;i<m_GlobalHeader.vcChecksum.size();i++) strFile += m_GlobalHeader.vcChecksum[i];

			s << "UVF::VerifyChecksum: checksum mismatch (stage 1). Should be " << strActual << " but is " << strFile << ".";
			*pstrProblem = s.str();
		}
		return false;
	}

	for (size_t i = 0;i<vecActualCheckSum.size();i++) 
		if (vecActualCheckSum[i] != m_GlobalHeader.vcChecksum[i]) {			
			if (pstrProblem != NULL)  {
				stringstream s;
				string strActual = "", strFile = "";
				for (size_t j = 0;j<vecActualCheckSum.size();j++) strActual += vecActualCheckSum[j];
				for (size_t j = 0;j<m_GlobalHeader.vcChecksum.size();j++) strFile += m_GlobalHeader.vcChecksum[j];

				s << "UVF::VerifyChecksum: checksum mismatch (stage 2). Should be " << strActual << " but is " << strFile << ".";
				*pstrProblem = s.str();
			}
			return false;
		}

	return true;
}


void UVF::ParseDataBlocks() {
	UINT64 iOffset = m_GlobalHeader.GetDataPos();
	do  {
		DataBlock *d = new DataBlock(&m_streamFile, iOffset, m_GlobalHeader.bIsBigEndian);

		// if we recongnize the block -> read it completely
		if (d->ulBlockSemantics > BS_EMPTY && d->ulBlockSemantics < BS_UNKNOWN) {
			BlockSemanticTable eTableID = d->ulBlockSemantics;
			delete d;
			d = CreateBlockFromSemanticEntry(eTableID, &m_streamFile, iOffset, m_GlobalHeader.bIsBigEndian);
		}

		iOffset += d->GetOffsetToNextBlock();

		m_DataBlocks.push_back(d);

	} while (m_DataBlocks[m_DataBlocks.size()-1]->ulOffsetToNextDataBlock != 0);
}

// ********************** file creation routines

void UVF::UpdateChecksum() {
  if (m_GlobalHeader.ulChecksumSemanticsEntry == CS_NONE) return;
	m_GlobalHeader.UpdateChecksum(ComputeChecksum(m_GlobalHeader.ulChecksumSemanticsEntry), &m_streamFile);
}

bool UVF::SetGlobalHeader(const GlobalHeader& globalHeader) {
	if (m_bFileIsLoaded) return false;

	m_GlobalHeader = globalHeader;
	
	// set the canonical data
	m_GlobalHeader.ulAdditionalHeaderSize = 0;
	m_GlobalHeader.ulOffsetToFirstDataBlock = 0;
	m_GlobalHeader.ulFileVersion = ms_ulReaderVersion;

	if (m_GlobalHeader.ulChecksumSemanticsEntry >= CS_UNKNOWN) m_GlobalHeader.ulChecksumSemanticsEntry = CS_NONE;

	if (m_GlobalHeader.ulChecksumSemanticsEntry > CS_NONE) {
		m_GlobalHeader.vcChecksum.resize(size_t(ChecksumElemLength(m_GlobalHeader.ulChecksumSemanticsEntry)));
	} else m_GlobalHeader.vcChecksum.resize(0);
	return true;
}

bool UVF::AddDataBlock(DataBlock& dataBlock, UINT64 iSizeofData) {

	if (!dataBlock.Verify(iSizeofData)) return false;

	DataBlock* d = dataBlock.Clone();
	d->ulOffsetToNextDataBlock = d->GetOffsetToNextBlock();
	m_DataBlocks.push_back(d);

	return true;
}

UINT64 UVF::ComputeNewFileSize() {
	UINT64 iFileSize = m_GlobalHeader.GetDataPos();
	for (size_t i = 0;i<m_DataBlocks.size();i++) 
		iFileSize += m_DataBlocks[i]->GetOffsetToNextBlock();
	return iFileSize;
}


bool UVF::Create() {
	if (m_bFileIsLoaded) return false;

  m_bFileIsLoaded = m_streamFile.Create();

	if (m_bFileIsLoaded) {

    m_bFileIsReadWrite = true;

    unsigned char pData[8];
		pData[0] = 'U';
		pData[1] = 'V';
		pData[2] = 'F';
		pData[3] = '-';
		pData[4] = 'D';
		pData[5] = 'A';
		pData[6] = 'T';
		pData[7] = 'A';
    m_streamFile.WriteRAW(pData, 8);

		m_GlobalHeader.CopyHeaderToFile(&m_streamFile);
    
		UINT64 iOffset = m_GlobalHeader.GetDataPos();
	  for (size_t i = 0;i<m_DataBlocks.size();i++)
		  	iOffset += m_DataBlocks[i]->CopyToFile(&m_streamFile, iOffset, m_GlobalHeader.bIsBigEndian, i == m_DataBlocks.size()-1);

		return true;
	}else {
		return false;
	}

}


