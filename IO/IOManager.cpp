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

/**
  \file    IOManager.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "IOManager.h"
#include <Controller/MasterController.h>
#include <IO/DICOM/DICOMParser.h>
#include <IO/KeyValueFileParser.h>
#include <Basics/SysTools.h>
#include <sstream>
#include <fstream>


using namespace std;

IOManager::IOManager(MasterController* masterController) :
  m_pMasterController(masterController)
{

}

IOManager::~IOManager()
{

}

vector<FileStackInfo*> IOManager::ScanDirectory(std::string strDirectory) {

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","Scanning directory %s", strDirectory.c_str());

  std::vector<FileStackInfo*> fileStacks;

  // right now we scan the directory only for DICOM files but in the future other image scanners will be added
  DICOMParser p;
  p.GetDirInfo(strDirectory);

  if (p.m_FileStacks.size() == 1)
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found a single DICOM stack");
  else
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found %i DICOM stacks", p.m_FileStacks.size());

  for (unsigned int iStackID = 0;iStackID < p.m_FileStacks.size();iStackID++) {    
    DICOMStackInfo* f = new DICOMStackInfo((DICOMStackInfo*)p.m_FileStacks[iStackID]);

    stringstream s;
    s << f->m_strFileType << " Stack: " << f->m_strDesc;
    f->m_strDesc = s.str();

    fileStacks.push_back(f);
  }

  // TODO: add other image parsers here

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  scan complete");

  return fileStacks;
}


bool IOManager::ConvertDATDataset(const std::string& strFilename, const std::string& strTargetFilename)
{
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDATDataset","Converting DAT dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());

  unsigned int	TypeID;
  UINT64			  iComponentSize=8;
  UINT64			  iComponentCount=1;
  UINTVECTOR3		vVolumeSize;
  FLOATVECTOR3	vVolumeAspect;
  string        strRAWFile;

  KeyValueFileParser parser(strFilename);

  if (parser.FileReadable())  {
	  KeyValPair* format         = parser.GetData("FORMAT");
	  if (format == NULL) 
		  return false;
	  else {
		  if (format->strValueUpper == "UCHAR" || format->strValueUpper == "BYTE") {
			  TypeID = 0;
			  iComponentSize = 8;
			  iComponentCount = 1;
		  } else if (format->strValueUpper == "USHORT") {
			  TypeID = 1;
			  iComponentSize = 16;
			  iComponentCount = 1;
		  } else if (format->strValueUpper == "UCHAR4") {
			  TypeID = 2;
			  iComponentSize = 32;
			  iComponentCount = 4;
		  } else if (format->strValueUpper == "FLOAT") {
			  TypeID = 3;
			  iComponentSize = 32;
			  iComponentCount = 1;
		  }
	  }

	  KeyValPair* objectfilename = parser.GetData("OBJECTFILENAME");
	  if (objectfilename == NULL) return false; else strRAWFile = objectfilename->strValue;

	  KeyValPair* resolution = parser.GetData("RESOLUTION");
	  if (resolution == NULL) return false; else vVolumeSize = resolution->vuiValue;

	  KeyValPair* sliceThickness = parser.GetData("SLICETHICKNESS");
	  if (sliceThickness == NULL) 
		  vVolumeAspect = FLOATVECTOR3(1,1,1);
	  else {			
		  vVolumeAspect = sliceThickness->vfValue;
		  vVolumeAspect = vVolumeAspect / vVolumeAspect.maxVal();
	  }

	  strRAWFile = SysTools::GetPath(strFilename) + strRAWFile;

    return ConvertRAWDataset(strRAWFile, strTargetFilename, iComponentSize, iComponentCount, 
                             vVolumeSize, vVolumeAspect, "Qvis data", SysTools::GetFilename(strFilename));

  } else return false;
}

bool IOManager::ConvertRAWDataset(const std::string& strFilename, const std::string& strTargetFilename,
				                         UINT64	iComponentSize, UINT64 iComponentCount,
                                 UINTVECTOR3 vVolumeSize,FLOATVECTOR3 vVolumeAspect, string strDesc, string strSource, UVFTables::ElementSemanticTable eType)
{
  m_pMasterController->DebugOut()->Message("IOManager::ConvertRAWDataset","Converting RAW dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());

  LargeRAWFile SourceData(strFilename);
  SourceData.Open(false);

  if (!SourceData.IsOpen()) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unable to open source file %s", strFilename.c_str());
    return false;
  }

	wstring wstrUVFName(strTargetFilename.begin(), strTargetFilename.end());
	UVF uvfFile(wstrUVFName);

	UINT64 iLodLevelCount = 1;
  unsigned int iMaxVal = vVolumeSize.maxVal();

  while (iMaxVal > BRICKSIZE) {
    iMaxVal /= 2;
    iLodLevelCount++;
  }

	GlobalHeader uvfGlobalHeader;
	uvfGlobalHeader.ulChecksumSemanticsEntry = UVFTables::CS_MD5;
	uvfFile.SetGlobalHeader(uvfGlobalHeader);

	RasterDataBlock dataVolume;

  if (strSource == "") 
    dataVolume.strBlockID = (strDesc!="") ? strDesc + "volume converted by ImageVis3D" : "Volume converted by ImageVis3D";
  else
    dataVolume.strBlockID = (strDesc!="") ? strDesc + " volume converted from " + strSource + " by ImageVis3D" : "Volume converted from " + strSource + " by ImageVis3D";

	dataVolume.ulCompressionScheme = UVFTables::COS_NONE;
	dataVolume.ulDomainSemantics.push_back(UVFTables::DS_X);
	dataVolume.ulDomainSemantics.push_back(UVFTables::DS_Y);
	dataVolume.ulDomainSemantics.push_back(UVFTables::DS_Z);

	dataVolume.ulDomainSize.push_back(vVolumeSize.x);
	dataVolume.ulDomainSize.push_back(vVolumeSize.y);
	dataVolume.ulDomainSize.push_back(vVolumeSize.z);

	dataVolume.ulLODDecFactor.push_back(2);
	dataVolume.ulLODDecFactor.push_back(2);
	dataVolume.ulLODDecFactor.push_back(2);

	dataVolume.ulLODGroups.push_back(0);
	dataVolume.ulLODGroups.push_back(0);
	dataVolume.ulLODGroups.push_back(0);

	dataVolume.ulLODLevelCount.push_back(iLodLevelCount);

	vector<UVFTables::ElementSemanticTable> vSem;

	switch (iComponentCount) {
		case 3 : vSem.push_back(UVFTables::ES_RED);
				 vSem.push_back(UVFTables::ES_GREEN);
				 vSem.push_back(UVFTables::ES_BLUE); break;
		case 4 : vSem.push_back(UVFTables::ES_RED);
				 vSem.push_back(UVFTables::ES_GREEN);
				 vSem.push_back(UVFTables::ES_BLUE); 
				 vSem.push_back(UVFTables::ES_ALPHA); break;
		default : for (uint i = 0;i<iComponentCount;i++) vSem.push_back(eType);
	}

	dataVolume.SetTypeToVector(iComponentSize/iComponentCount, 
							               iComponentSize == 32 ? 23 : iComponentSize/iComponentCount,
							               iComponentSize == 32, 
							               vSem);
	
	dataVolume.ulBrickSize.push_back(BRICKSIZE);
	dataVolume.ulBrickSize.push_back(BRICKSIZE);
	dataVolume.ulBrickSize.push_back(BRICKSIZE);

	dataVolume.ulBrickOverlap.push_back(BRICKOVERLAP);
	dataVolume.ulBrickOverlap.push_back(BRICKOVERLAP);
	dataVolume.ulBrickOverlap.push_back(BRICKOVERLAP);

	vector<double> vScale;
	vScale.push_back(vVolumeAspect.x);
	vScale.push_back(vVolumeAspect.y);
	vScale.push_back(vVolumeAspect.z);
	dataVolume.SetScaleOnlyTransformation(vScale);

	switch (iComponentSize) {
		case 8 :	switch (iComponentCount) {
						case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,1>); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,2>); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,3>); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,4>); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		case 16 :		switch (iComponentCount) {
						case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,1>); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,2>); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,3>); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,4>); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		case 32 :	switch (iComponentCount) {
						case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,1>); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,2>); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,3>); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,4>); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentSize %i.", iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
	}

	string strProblemDesc;
	if (!dataVolume.Verify(&strProblemDesc)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Verify failed with the following reason: %s", strProblemDesc.c_str()); 
    uvfFile.Close(); 
    SourceData.Close();
		return false;
	}

	if (!uvfFile.AddDataBlock(&dataVolume,dataVolume.ComputeDataSize(), true)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","AddDataBlock failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
		return false;
	}

 	Histogram1DDataBlock Histogram1D;
  if (!Histogram1D.Compute(&dataVolume)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Computation of 1D Histogram failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
		return false;
  }
  Histogram2DDataBlock Histogram2D;
  if (!Histogram2D.Compute(&dataVolume)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Computation of 2D Histogram failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
		return false;
  }

	uvfFile.AddDataBlock(&Histogram1D,Histogram1D.ComputeDataSize());
	uvfFile.AddDataBlock(&Histogram2D,Histogram2D.ComputeDataSize());

/*
  // TODO: maybe add information from the source file to the UVF, like DICOM desc etc.

  KeyValuePairDataBlock testPairs;
	testPairs.AddPair("SOURCE","DICOM");
	testPairs.AddPair("CONVERTED BY","DICOM2UVF V1.0");
	UINT64 iDataSize = testPairs.ComputeDataSize();
	uvfFile.AddDataBlock(testPairs,iDataSize);
*/

	uvfFile.Create();
	SourceData.Close();
	uvfFile.Close();

  return true;
}

bool IOManager::ConvertDataset(FileStackInfo* pStack, const std::string& strTargetFilename) {
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","Request to convert stack of %s files to %s received", pStack->m_strDesc.c_str(), strTargetFilename.c_str());

  if (pStack->m_strFileType == "DICOM") {
    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Detected DICOM stack, starting DICOM conversion");

    DICOMStackInfo* pDICOMStack = ((DICOMStackInfo*)pStack);

		m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Stack contains %i files",  pDICOMStack->m_Elements.size());
		m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Series: %i  Bits: %i (%i)", pDICOMStack->m_iSeries, pDICOMStack->m_iAllocated, pDICOMStack->m_iStored);
		m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Date: %s  Time: %s", pDICOMStack->m_strAcquDate.c_str(), pDICOMStack->m_strAcquTime.c_str());
		m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Modality: %s  Description: %s", pDICOMStack->m_strModality.c_str(), pDICOMStack->m_strDesc.c_str());
		m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Aspect Ratio: %g %g %g", pDICOMStack->m_fvfAspect.x, pDICOMStack->m_fvfAspect.y, pDICOMStack->m_fvfAspect.z);

    string strTempMergeFilename = strTargetFilename + "~";

    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Creating intermediate file %s", strTempMergeFilename.c_str()); 

		ofstream fs;
		fs.open(strTempMergeFilename.c_str(),fstream::binary);
		if (fs.fail())  {
			m_pMasterController->DebugOut()->Error("IOManager::ConvertDataset","Could not create temp file %s aborted conversion.", strTempMergeFilename.c_str()); 
			return false;
		}

		char *pData = NULL;
		for (uint j = 0;j<pDICOMStack->m_Elements.size();j++) {
			pDICOMStack->m_Elements[j]->GetData((void**)&pData); // the first call does a "new" on pData 

			unsigned int iDataSize = pDICOMStack->m_Elements[j]->GetDataSize();

			if (pDICOMStack->m_bIsBigEndian) {
				switch (pDICOMStack->m_iAllocated) {
					case  8 : break;
					case 16 : {
								for (uint k = 0;k<iDataSize/2;k++)
									((short*)pData)[k] = EndianConvert::Swap<short>(((short*)pData)[k]);
							  } break;
					case 32 : {
								for (uint k = 0;k<iDataSize/4;k++)
									((float*)pData)[k] = EndianConvert::Swap<float>(((float*)pData)[k]);
							  } break;
				}
			}

      // HACK: this code assumes 3 component data is always 3*char
			if (pDICOMStack->m_iComponentCount == 3) {
				unsigned int iRGBADataSize = (iDataSize / 3 ) * 4;
				
				char *pRGBAData = new char[ iRGBADataSize ];
				for (uint k = 0;k<iDataSize/3;k++) {
					pRGBAData[k*4+0] = pData[k*3+0];
					pRGBAData[k*4+1] = pData[k*3+1];
					pRGBAData[k*4+2] = pData[k*3+2];
					pRGBAData[k*4+3] = char(255);
				}

				fs.write(pRGBAData, iRGBADataSize);
				delete [] pRGBAData;				
			} else {
				fs.write(pData, iDataSize);
			}
		}
		delete [] pData;

		fs.close();
    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    done creating intermediate file %s", strTempMergeFilename.c_str()); 

		UINTVECTOR3 iSize = pDICOMStack->m_ivSize;
		iSize.z *= (unsigned int)pDICOMStack->m_Elements.size();

    // TODO: evaluate pDICOMStack->m_strModality

    bool result = ConvertRAWDataset(strTempMergeFilename, strTargetFilename, pDICOMStack->m_iAllocated, 
                                    pDICOMStack->m_iComponentCount, iSize, pDICOMStack->m_fvfAspect, 
                                    "DICOM stack", SysTools::GetFilename(pDICOMStack->m_Elements[0]->m_strFileName)
                                    + " to " + SysTools::GetFilename(pDICOMStack->m_Elements[pDICOMStack->m_Elements.size()-1]->m_strFileName));

    if( remove(strTempMergeFilename.c_str()) != 0 ) {
      m_pMasterController->DebugOut()->Warning("IOManager::ConvertDataset","Unable to remove temp file %s", strTempMergeFilename.c_str());
    }

    return result;
  }

  // TODO: more stack converters here

  m_pMasterController->DebugOut()->Error("IOManager::ConvertDataset","Unknown source stack type %s", pStack->m_strFileType.c_str());
  return false;
}

bool IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename) {
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","Request to convert dataset %s to %s received.", strFilename.c_str(), strTargetFilename.c_str());

  if (SysTools::ToLowerCase(SysTools::GetExt(strFilename)) == "dat") {
    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Detected QVis DAT file forwarding to DAT converter.");
    return ConvertDATDataset(strFilename, strTargetFilename);
  } else {
    // TODO: more converters here
    return false;
  }
}

VolumeDataset* IOManager::ConvertDataset(FileStackInfo* pStack, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(pStack, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(strFilename, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::LoadDataset(std::string strFilename, AbstrRenderer* requester) {
  return m_pMasterController->MemMan()->LoadDataset(strFilename, requester);
}

bool IOManager::NeedsConversion(std::string strFilename, bool& bChecksumFail) {
  wstring wstrFilename(strFilename.begin(), strFilename.end());
  return !UVF::IsUVFFile(wstrFilename, bChecksumFail);
}