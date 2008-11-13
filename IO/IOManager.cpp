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
#include <IO/Images/ImageParser.h>
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

  DICOMParser parseDICOM;
  parseDICOM.GetDirInfo(strDirectory);

  if (parseDICOM.m_FileStacks.size() == 1)
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found a single DICOM stack");
  else
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found %i DICOM stacks", parseDICOM.m_FileStacks.size());

  for (unsigned int iStackID = 0;iStackID < parseDICOM.m_FileStacks.size();iStackID++) {    
    DICOMStackInfo* f = new DICOMStackInfo((DICOMStackInfo*)parseDICOM.m_FileStacks[iStackID]);

    stringstream s;
    s << f->m_strFileType << " Stack: " << f->m_strDesc;
    f->m_strDesc = s.str();

    fileStacks.push_back(f);
  }


  ImageParser parseImages;
  parseImages.GetDirInfo(strDirectory);

  if (parseImages.m_FileStacks.size() == 1)
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found a single image stack");
  else
    m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  found %i image stacks", parseImages.m_FileStacks.size());

  for (unsigned int iStackID = 0;iStackID < parseImages.m_FileStacks.size();iStackID++) {    
    ImageStackInfo* f = new ImageStackInfo((ImageStackInfo*)parseImages.m_FileStacks[iStackID]);

    stringstream s;
    s << f->m_strFileType << " Stack: " << f->m_strDesc;
    f->m_strDesc = s.str();

    fileStacks.push_back(f);
  }

  /// \todo  add other image parsers here

  m_pMasterController->DebugOut()->Message("IOManager::ScanDirectory","  scan complete");

  return fileStacks;
}

bool IOManager::ConvertNHDRDataset(const std::string& strFilename, const std::string& strTargetFilename) 
{
  m_pMasterController->DebugOut()->Message("IOManager::ConvertNHDRDataset","Converting NRHD dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());

  // Check Magic value in NRRD File first
	ifstream fileData(strFilename.c_str());	
  string strFirstLine;

	if (fileData.is_open())
	{
		getline (fileData,strFirstLine);
    if (strFirstLine.substr(0,7) != "NRRD000") {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","The file %s is not a NRRD file (missing magic)", strFilename.c_str());
      return false;
    }
  } else {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open NRHD file %s", strFilename.c_str());
    return false;
  }
  fileData.close();


  // read data
  UINT64			  iComponentSize=8;
  UINT64			  iComponentCount=1;
  bool          bSigned=false;
  bool          bBigEndian=false;
  UINTVECTOR3		vVolumeSize;
  FLOATVECTOR3	vVolumeAspect(1,1,1);
  string        strRAWFile;

  KeyValueFileParser parser(strFilename);

  if (!parser.FileReadable()) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open NRHD file %s", strFilename.c_str());
    return false;
  }

  KeyValPair* kvpType = parser.GetData("TYPE");
  if (kvpType == NULL) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open find token \"type\" in file %s", strFilename.c_str());
	  return false;
  } else {
	  if (kvpType->strValueUpper == "SIGNED CHAR" || kvpType->strValueUpper == "INT8" || kvpType->strValueUpper == "INT8_T") {
		  bSigned = true;
      iComponentSize = 8;
	  } else if (kvpType->strValueUpper == "UCHAR" || kvpType->strValueUpper == "UNSIGNED CHAR" ||  kvpType->strValueUpper == "UINT8" || kvpType->strValueUpper == "UINT8_T") {
		  bSigned = false;
      iComponentSize = 8;
	  } else if (kvpType->strValueUpper == "SHORT" || kvpType->strValueUpper == "SHORT INT" ||  kvpType->strValueUpper == "SIGNED SHORT" || kvpType->strValueUpper == "SIGNED SHORT INT" || kvpType->strValueUpper == "INT16" || kvpType->strValueUpper == "INT16_T") {
		  bSigned = true;
		  iComponentSize = 16;
	  } else if (kvpType->strValueUpper == "USHORT" || kvpType->strValueUpper == "UNSIGNED SHORT" || kvpType->strValueUpper == "UNSIGNED SHORT INT" || kvpType->strValueUpper == "UINT16" || kvpType->strValueUpper == "UINT16_T") {
		  bSigned = false;
		  iComponentSize = 16;
	  } else if (kvpType->strValueUpper == "INT" || kvpType->strValueUpper == "SIGNED INT" || kvpType->strValueUpper == "INT32" || kvpType->strValueUpper == "INT32_T") {
		  bSigned = true;
		  iComponentSize = 32;
	  } else if (kvpType->strValueUpper == "UINT" || kvpType->strValueUpper == "UNSIGNED INT" || kvpType->strValueUpper == "UINT32" || kvpType->strValueUpper == "UINT32_T") {
		  bSigned = false;
		  iComponentSize = 32;
	  } else if (kvpType->strValueUpper == "LONGLONG" || kvpType->strValueUpper == "LONG LONG" || kvpType->strValueUpper == "LONG LONG INT" || kvpType->strValueUpper == "SIGNED LONG LONG" || kvpType->strValueUpper == "SIGNED LONG LONG INT" || kvpType->strValueUpper == "INT64" || kvpType->strValueUpper == "INT64_T") {
		  bSigned = true;
		  iComponentSize = 64;
	  } else if (kvpType->strValueUpper == "ULONGLONG" || kvpType->strValueUpper == "UNSIGNED LONG LONG" || kvpType->strValueUpper == "UNSIGNED LONG LONG INT" || kvpType->strValueUpper == "UINT64" || kvpType->strValueUpper == "UINT64_T") {
		  bSigned = true;
		  iComponentSize = 64;
	  } else if (kvpType->strValueUpper == "FLOAT") {
		  bSigned = true;
		  iComponentSize = 32;
	  } else if (kvpType->strValueUpper == "DOUBLE") {
		  bSigned = true;
		  iComponentSize = 64;
	  } else {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Unsupported \"type\" in file %s", strFilename.c_str());
	    return false;
    }
  }

  KeyValPair* kvpDim = parser.GetData("DIMENSION");
  if (kvpDim == NULL) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open find token \"dimension\" in file %s", strFilename.c_str());
	  return false;
  } else {
    if (kvpDim->iValue != 3)  {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Only 3D nhdr are supported at the moment");
	    return false;
     }
  }

  KeyValPair* kvpSizes = parser.GetData("SIZES");
  if (kvpSizes == NULL) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open find token \"sizes\" in file %s", strFilename.c_str());
	  return false;
  } else {
    vVolumeSize = kvpSizes->vuiValue;
  }

  KeyValPair* kvpDataFile = parser.GetData("DATA FILE");
  if (kvpDataFile == NULL) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open find token \"data file\" in file %s", strFilename.c_str());
	  return false;
  } else {
    strRAWFile = SysTools::GetPath(strFilename) + kvpDataFile->strValue;
  }
  
  KeyValPair* kvpEncoding = parser.GetData("ENCODING");
  if (kvpEncoding == NULL) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Could not open find token \"encoding\" in file %s", strFilename.c_str());
	  return false;
  } else {
    if (kvpEncoding->strValueUpper != "RAW")  {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertNHDRDataset","Only raw encodings are supported at the moment.");
	    return false;
     }
  }

  KeyValPair* kvpSpacings = parser.GetData("SPACINGS");
  if (kvpSpacings != NULL) {
    vVolumeAspect = kvpSpacings->vfValue;
  }

  KeyValPair* kvpEndian = parser.GetData("ENDIAN");
  if (kvpEndian != NULL && kvpEndian->strValueUpper == "BIG") bBigEndian = true;

  return ConvertRAWDataset(strRAWFile, strTargetFilename, iComponentSize, iComponentCount, bSigned, bBigEndian != EndianConvert::IsBigEndian(),
                           vVolumeSize, vVolumeAspect, "NRRD data", SysTools::GetFilename(strFilename));

}

bool IOManager::ConvertDATDataset(const std::string& strFilename, const std::string& strTargetFilename)
{
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDATDataset","Converting DAT dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());

  UINT64			  iComponentSize=8;
  UINT64			  iComponentCount=1;
  bool          bSigned=false;
  UINTVECTOR3		vVolumeSize;
  FLOATVECTOR3	vVolumeAspect;
  string        strRAWFile;

  KeyValueFileParser parser(strFilename);

  if (parser.FileReadable())  {
	  KeyValPair* format = parser.GetData("FORMAT");
	  if (format == NULL) 
		  return false;
	  else {
		  if (format->strValueUpper == "UCHAR" || format->strValueUpper == "BYTE") {
  		  bSigned = false;
			  iComponentSize = 8;
			  iComponentCount = 1;
		  } else if (format->strValueUpper == "USHORT") {
  		  bSigned = false;
			  iComponentSize = 16;
			  iComponentCount = 1;
		  } else if (format->strValueUpper == "UCHAR4") {
  		  bSigned = false;
			  iComponentSize = 32;
			  iComponentCount = 4;
		  } else if (format->strValueUpper == "FLOAT") {
  		  bSigned = true;
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

    /// \todo  detect big endian DAT/RAW combinations and set the conversion parameter accordingly instead of always converting if the machine is big endian 
    return ConvertRAWDataset(strRAWFile, strTargetFilename, iComponentSize, iComponentCount, bSigned, EndianConvert::IsBigEndian(),
                             vVolumeSize, vVolumeAspect, "Qvis data", SysTools::GetFilename(strFilename));

  } else return false;
}

bool IOManager::ConvertRAWDataset(const std::string& strFilename, const std::string& strTargetFilename,
				                         UINT64	iComponentSize, UINT64 iComponentCount, bool bSigned, bool bConvertEndianness,
                                 UINTVECTOR3 vVolumeSize,FLOATVECTOR3 vVolumeAspect, string strDesc, string strSource, UVFTables::ElementSemanticTable eType)
{
  if (iComponentSize < 16) bConvertEndianness = false; // catch silly user input

  m_pMasterController->DebugOut()->Message("IOManager::ConvertRAWDataset","Converting RAW dataset %s to %s", strFilename.c_str(), strTargetFilename.c_str());

  string strSourceFilename;
  string tmpFilename = SysTools::GetPath(strTargetFilename)+SysTools::GetFilename(strFilename)+".endianess";
  if (bConvertEndianness) {
    m_pMasterController->DebugOut()->Message("IOManager::ConvertRAWDataset","Performaing endianess conversion of RAW dataset %s to %s", strFilename.c_str(), tmpFilename.c_str());

    if (iComponentSize != 16 && iComponentSize != 32 && iComponentSize != 64) {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unable to endian convert anything but 16bit, 32bit, or 64bit values (requested %i)", iComponentSize);
      return false;
    }


    LargeRAWFile WrongEndianData(strFilename);
    WrongEndianData.Open(false);

    if (!WrongEndianData.IsOpen()) {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unable to open source file %s", strFilename.c_str());
      return false;
    }

    LargeRAWFile ConvEndianData(tmpFilename);
    ConvEndianData.Create();

    if (!ConvEndianData.IsOpen()) {
      m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unable to open temp file %s for endianess conversion", tmpFilename.c_str());
      WrongEndianData.Close();
      return false;
    }

    UINT64 ulFileLength = WrongEndianData.GetCurrentSize();
    size_t iBufferSize = min<size_t>(size_t(ulFileLength), size_t(BRICKSIZE*BRICKSIZE*BRICKSIZE*iComponentSize/8)); // this must fit into memory otherwise other subsystems would break
    UINT64 ulBufferConverted = 0;

    unsigned char* pBuffer = new unsigned char[iBufferSize];

    while (ulBufferConverted < ulFileLength) {

      size_t iBytesRead = WrongEndianData.ReadRAW(pBuffer, iBufferSize);

      switch (iComponentSize) {
        case 16 : for (size_t i = 0;i<iBytesRead;i+=2) 
                    EndianConvert::Swap<unsigned short>((unsigned short*)(pBuffer+i)); 
                  break;
        case 32 : for (size_t i = 0;i<iBytesRead;i+=4) 
                    EndianConvert::Swap<float>((float*)(pBuffer+i)); 
                  break;
        case 64 : for (size_t i = 0;i<iBytesRead;i+=8) 
                    EndianConvert::Swap<double>((double*)(pBuffer+i)); 
                  break;
      }

      size_t iBytesWritten = ConvEndianData.WriteRAW(pBuffer, iBytesRead);

      if (iBytesRead != iBytesWritten)  {
        m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Read/Write error converting endianess from %s to %s", strFilename.c_str(), tmpFilename.c_str());
        WrongEndianData.Close();
        ConvEndianData.Close();
        remove(tmpFilename.c_str());
        delete [] pBuffer;
        return false;
      }

      ulBufferConverted += UINT64(iBytesWritten);
    }

    delete [] pBuffer;

    WrongEndianData.Close();
    ConvEndianData.Close();
    strSourceFilename = tmpFilename;
  } else strSourceFilename = strFilename;


  LargeRAWFile SourceData(strSourceFilename);
  SourceData.Open(false);

  if (!SourceData.IsOpen()) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unable to open source file %s", strSourceFilename.c_str());
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
  uvfGlobalHeader.bIsBigEndian = EndianConvert::IsBigEndian();
	uvfGlobalHeader.ulChecksumSemanticsEntry = UVFTables::CS_MD5;
	uvfFile.SetGlobalHeader(uvfGlobalHeader);

	RasterDataBlock dataVolume;

  if (strSource == "") 
    dataVolume.strBlockID = (strDesc!="") ? strDesc + " volume converted by ImageVis3D" : "Volume converted by ImageVis3D";
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
							               bSigned, 
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
    case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,1>, m_pMasterController->DebugOut()); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,2>, m_pMasterController->DebugOut()); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,3>, m_pMasterController->DebugOut()); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned char,4>, m_pMasterController->DebugOut()); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		case 16 :		switch (iComponentCount) {
						case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,1>, m_pMasterController->DebugOut()); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,2>, m_pMasterController->DebugOut()); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,3>, m_pMasterController->DebugOut()); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<unsigned short,4>, m_pMasterController->DebugOut()); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		case 32 :	switch (iComponentCount) {
						case 1 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,1>, m_pMasterController->DebugOut()); break;
						case 2 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,2>, m_pMasterController->DebugOut()); break;
						case 3 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,3>, m_pMasterController->DebugOut()); break;
						case 4 : dataVolume.FlatDataToBrickedLOD(&SourceData, "tempFile.tmp", CombineAverage<float,4>, m_pMasterController->DebugOut()); break;
						default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentCount %i for iComponentSize %i.", iComponentCount, iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
					} break;
		default: m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Unsupported iComponentSize %i.", iComponentSize); uvfFile.Close(); SourceData.Close(); return false;
	}

	string strProblemDesc;
	if (!dataVolume.Verify(&strProblemDesc)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Verify failed with the following reason: %s", strProblemDesc.c_str()); 
    uvfFile.Close(); 
    SourceData.Close();
    if (bConvertEndianness) remove(tmpFilename.c_str());
		return false;
	}

	if (!uvfFile.AddDataBlock(&dataVolume,dataVolume.ComputeDataSize(), true)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","AddDataBlock failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
    if (bConvertEndianness) remove(tmpFilename.c_str());
		return false;
	}

 	Histogram1DDataBlock Histogram1D;
  if (!Histogram1D.Compute(&dataVolume)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Computation of 1D Histogram failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
    if (bConvertEndianness) remove(tmpFilename.c_str());
		return false;
  }
  Histogram2DDataBlock Histogram2D;
  if (!Histogram2D.Compute(&dataVolume)) {
    m_pMasterController->DebugOut()->Error("IOManager::ConvertRAWDataset","Computation of 2D Histogram failed!"); 
    uvfFile.Close(); 
    SourceData.Close();
    if (bConvertEndianness) remove(tmpFilename.c_str());
		return false;
  }

	uvfFile.AddDataBlock(&Histogram1D,Histogram1D.ComputeDataSize());
	uvfFile.AddDataBlock(&Histogram2D,Histogram2D.ComputeDataSize());

/*
  /// \todo maybe add information from the source file to the UVF, like DICOM desc etc.

  KeyValuePairDataBlock testPairs;
	testPairs.AddPair("SOURCE","DICOM");
	testPairs.AddPair("CONVERTED BY","DICOM2UVF V1.0");
	UINT64 iDataSize = testPairs.ComputeDataSize();
	uvfFile.AddDataBlock(testPairs,iDataSize);
*/

	uvfFile.Create();
	SourceData.Close();
	uvfFile.Close();
  if (bConvertEndianness) remove(tmpFilename.c_str());

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

    /// \todo evaluate pDICOMStack->m_strModality

    bool result = ConvertRAWDataset(strTempMergeFilename, strTargetFilename, pDICOMStack->m_iAllocated, 
                                    pDICOMStack->m_iComponentCount, 
                                    false,
                                    pDICOMStack->m_bIsBigEndian != EndianConvert::IsBigEndian(),
                                    iSize, pDICOMStack->m_fvfAspect, 
                                    "DICOM stack", SysTools::GetFilename(pDICOMStack->m_Elements[0]->m_strFileName)
                                    + " to " + SysTools::GetFilename(pDICOMStack->m_Elements[pDICOMStack->m_Elements.size()-1]->m_strFileName));

    if( remove(strTempMergeFilename.c_str()) != 0 ) {
      m_pMasterController->DebugOut()->Warning("IOManager::ConvertDataset","Unable to remove temp file %s", strTempMergeFilename.c_str());
    }

    return result;
  } else {
     if (pStack->m_strFileType == "IMAGE") {
        m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Detected Image stack, starting image conversion");
        m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Stack contains %i files",  pStack->m_Elements.size());

        string strTempMergeFilename = strTargetFilename + "~";
        m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    Creating intermediate file %s", strTempMergeFilename.c_str()); 

        ofstream fs;
        fs.open(strTempMergeFilename.c_str(),fstream::binary);
        if (fs.fail())  {
	        m_pMasterController->DebugOut()->Error("IOManager::ConvertDataset","Could not create temp file %s aborted conversion.", strTempMergeFilename.c_str()); 
	        return false;
        }

	      char *pData = NULL;
	      for (uint j = 0;j<pStack->m_Elements.size();j++) {
          pStack->m_Elements[j]->GetData((void**)&pData); // the first call does a "new" on pData 

          unsigned int iDataSize = pStack->m_Elements[j]->GetDataSize();
          fs.write(pData, iDataSize);
        }
        delete [] pData;


		    fs.close();
        m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","    done creating intermediate file %s", strTempMergeFilename.c_str()); 

		    UINTVECTOR3 iSize = pStack->m_ivSize;
		    iSize.z *= (unsigned int)pStack->m_Elements.size();

        bool result = ConvertRAWDataset(strTempMergeFilename, strTargetFilename, pStack->m_iAllocated, 
                                        pStack->m_iComponentCount, 
                                        false,
                                        pStack->m_bIsBigEndian != EndianConvert::IsBigEndian(),
                                        iSize, pStack->m_fvfAspect, 
                                        "Image stack", SysTools::GetFilename(pStack->m_Elements[0]->m_strFileName)
                                        + " to " + SysTools::GetFilename(pStack->m_Elements[pStack->m_Elements.size()-1]->m_strFileName));

        if( remove(strTempMergeFilename.c_str()) != 0 ) {
          m_pMasterController->DebugOut()->Warning("IOManager::ConvertDataset","Unable to remove temp file %s", strTempMergeFilename.c_str());
        }

        return result;
     }
  }


  m_pMasterController->DebugOut()->Error("IOManager::ConvertDataset","Unknown source stack type %s", pStack->m_strFileType.c_str());
  return false;
}

bool IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename) {
  m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","Request to convert dataset %s to %s received.", strFilename.c_str(), strTargetFilename.c_str());

  if (SysTools::ToLowerCase(SysTools::GetExt(strFilename)) == "dat") {
    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Detected QVis DAT file forwarding to DAT converter.");
    return ConvertDATDataset(strFilename, strTargetFilename);
  }

  if (SysTools::ToLowerCase(SysTools::GetExt(strFilename)) == "nhdr") {
    m_pMasterController->DebugOut()->Message("IOManager::ConvertDataset","  Detected NRRD header file forwarding to NHDR converter.");
    return ConvertNHDRDataset(strFilename, strTargetFilename);
  } 

  /// \todo more converters here
  return false;  
}

VolumeDataset* IOManager::ConvertDataset(FileStackInfo* pStack, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(pStack, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::ConvertDataset(const std::string& strFilename, const std::string& strTargetFilename, AbstrRenderer* requester) {
  if (!ConvertDataset(strFilename, strTargetFilename)) return NULL;
  return LoadDataset(strTargetFilename, requester);
}

VolumeDataset* IOManager::LoadDataset(const std::string& strFilename, AbstrRenderer* requester) {
  return m_pMasterController->MemMan()->LoadDataset(strFilename, requester);
}

bool IOManager::NeedsConversion(const std::string& strFilename, bool& bChecksumFail) {
  wstring wstrFilename(strFilename.begin(), strFilename.end());
  return !UVF::IsUVFFile(wstrFilename, bChecksumFail);
}

bool IOManager::NeedsConversion(const std::string& strFilename) {
  wstring wstrFilename(strFilename.begin(), strFilename.end());
  return !UVF::IsUVFFile(wstrFilename);
}
