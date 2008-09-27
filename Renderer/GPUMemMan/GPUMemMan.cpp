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
  \file    GPUMemMan.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/

#include "GPUMemMan.h"
#include "../../IO/Images/BMPLoader.h"
#include "../../Controller/MasterController.h"

using namespace std;

GPUMemMan::GPUMemMan(MasterController* masterController) :
  m_MasterController(masterController),
  m_SystemInfo(masterController->SysInfo()),
  m_iAllocatedGPUMemory(0),
  m_iAllocatedCPUMemory(0)
{

}

GPUMemMan::~GPUMemMan() {
  for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
    m_MasterController->DebugOut()->Warning("GPUMemMan::~GPUMemMan","Detected unfreed dataset %s.", i->pVolumeDataset->Filename().c_str());
  }

  for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
    m_MasterController->DebugOut()->Warning("GPUMemMan::~GPUMemMan","Detected unfreed SimpleTexture %s.", i->strFilename.c_str());
  }

  for (Trans1DListIter i = m_vpTrans1DList.begin();i<m_vpTrans1DList.end();i++) {
    m_MasterController->DebugOut()->Warning("GPUMemMan::~GPUMemMan","Detected unfreed 1D Transferfunction.");
  }

  for (Trans2DListIter i = m_vpTrans2DList.begin();i<m_vpTrans2DList.end();i++) {
    m_MasterController->DebugOut()->Warning("GPUMemMan::~GPUMemMan","Detected unfreed 2D Transferfunction.");
  }

  for (Texture3DListIter i = m_vpTex3DList.begin();i<m_vpTex3DList.end();i++) delete (*i);
  m_vpTex3DList.clear();
}

// ******************** Datasets

VolumeDataset* GPUMemMan::LoadDataset(const std::string& strFilename, AbstrRenderer* requester) {
  for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
    if (i->pVolumeDataset->Filename() == strFilename) {
      m_MasterController->DebugOut()->Message("GPUMemMan::LoadDataset","Reusing %s", strFilename.c_str());
      i->qpUser.push_back(requester);
      return i->pVolumeDataset;
    }
  }

  m_MasterController->DebugOut()->Message("GPUMemMan::LoadDataset","Loading %s", strFilename.c_str());
  VolumeDataset* dataset = new VolumeDataset(strFilename, false, m_MasterController);  // PRECONDITION: we assume that the file has been verified before
  if (dataset->IsOpen()) {
    m_vpVolumeDatasets.push_back(VolDataListElem(dataset, requester));
    return dataset;
  } else {
    m_MasterController->DebugOut()->Error("GPUMemMan::LoadDataset","Error opening dataset %s", strFilename.c_str());
    return NULL;
  }
}


void GPUMemMan::FreeDataset(VolumeDataset* pVolumeDataset, AbstrRenderer* requester) {
  for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
    if (i->pVolumeDataset == pVolumeDataset) {
      for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
        if (*j == requester) {
          i->qpUser.erase(j);
          if (i->qpUser.size() == 0) {
            m_MasterController->DebugOut()->Message("GPUMemMan::FreeDataset","Released Dataset %s", pVolumeDataset->Filename().c_str());
            delete pVolumeDataset;
            m_vpVolumeDatasets.erase(i);
          } else {
            m_MasterController->DebugOut()->Message("GPUMemMan::FreeDataset","Decreased access count but dataset %s is still in use by another subsystem", pVolumeDataset->Filename().c_str());
          }
          return;
        }
      }
    }
  }
  m_MasterController->DebugOut()->Warning("GPUMemMan::FreeDataset","Dataset %s not found or not being used by requester", pVolumeDataset->Filename().c_str());
}

// ******************** Simple Textures

GLTexture2D* GPUMemMan::Load2DTextureFromFile(const std::string& strFilename) {
  for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
    if (i->strFilename == strFilename) {
      m_MasterController->DebugOut()->Message("GPUMemMan::Load2DTextureFromFile","Reusing %s", strFilename.c_str());
      i->iAccessCounter++;
      return i->pTexture;
    }
  }

  TextureImage image;
  if (!BMPLoader::Load(strFilename, &image)) {
    m_MasterController->DebugOut()->Error("GPUMemMan::Load2DTextureFromFile","Unable to load file %s", strFilename.c_str());
    return NULL;
  }
  m_MasterController->DebugOut()->Message("GPUMemMan::Load2DTextureFromFile","Loading %s", strFilename.c_str());

  GLTexture2D* tex = new GLTexture2D(image.width,image.height, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, image.data, GL_LINEAR, GL_LINEAR);
  delete [] image.data;
  
  m_iAllocatedGPUMemory += tex->GetCPUSize();
  m_iAllocatedCPUMemory += tex->GetGPUSize();

  m_vpSimpleTextures.push_back(SimpleTextureListElem(1,tex,strFilename));
  return tex;
}


void GPUMemMan::FreeTexture(GLTexture2D* pTexture) {
  for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
    if (i->pTexture == pTexture) {
      i->iAccessCounter--;
      if (i->iAccessCounter == 0) {
        m_MasterController->DebugOut()->Message("GPUMemMan::FreeTexture","Deleted texture %s", i->strFilename.c_str());

        m_iAllocatedGPUMemory += i->pTexture->GetCPUSize();
        m_iAllocatedCPUMemory += i->pTexture->GetGPUSize();

        i->pTexture->Delete();
        m_vpSimpleTextures.erase(i);
      } else {
        m_MasterController->DebugOut()->Message("GPUMemMan::FreeTexture","Decreased access count but the texture %s is still in use by another subsystem",i->strFilename.c_str());
      }
      return;
    }
  }
  m_MasterController->DebugOut()->Warning("GPUMemMan::FreeTexture","Texture not found");
}

// ******************** 1D Trans

void GPUMemMan::Changed1DTrans(AbstrRenderer* requester, TransferFunction1D* pTransferFunction1D) {
  m_MasterController->DebugOut()->Message("GPUMemMan::Changed1DTrans","Sending change notification for 1D transfer function");

  for (Trans1DListIter i = m_vpTrans1DList.begin();i<m_vpTrans1DList.end();i++) {
    if (i->pTransferFunction1D == pTransferFunction1D) {
      for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
        if (*j != requester) (*j)->Changed1DTrans();
      }
    }
  }
}

void GPUMemMan::GetEmpty1DTrans(size_t iSize, AbstrRenderer* requester, TransferFunction1D** ppTransferFunction1D, GLTexture1D** tex) {
  m_MasterController->DebugOut()->Message("GPUMemMan::GetEmpty1DTrans","Creating new empty 1D transfer function");
  *ppTransferFunction1D = new TransferFunction1D(iSize);
  *tex = new GLTexture1D(GLuint(iSize), GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

  m_vpTrans1DList.push_back(Trans1DListElem(*ppTransferFunction1D, *tex, requester));
}

void GPUMemMan::Get1DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction1D** ppTransferFunction1D, GLTexture1D** tex) {
  m_MasterController->DebugOut()->Message("GPUMemMan::Get1DTransFromFile","Loading 2D transfer function from file");
  *ppTransferFunction1D = new TransferFunction1D(strFilename);
  *tex = new GLTexture1D(GLuint((*ppTransferFunction1D)->vColorData.size()), GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

  m_vpTrans1DList.push_back(Trans1DListElem(*ppTransferFunction1D, *tex, requester));
}

GLTexture1D* GPUMemMan::Access1DTrans(TransferFunction1D* pTransferFunction1D, AbstrRenderer* requester) {
  for (Trans1DListIter i = m_vpTrans1DList.begin();i<m_vpTrans1DList.end();i++) {
    if (i->pTransferFunction1D == pTransferFunction1D) {
      m_MasterController->DebugOut()->Message("GPUMemMan::Access1DTrans","Accessing 1D transferfunction");
      i->qpUser.push_back(requester);
      return i->pTexture;
    }
  }

  m_MasterController->DebugOut()->Error("GPUMemMan::Access1DTrans","Unable to find 1D transferfunction");
  return NULL;
}

void GPUMemMan::Free1DTrans(TransferFunction1D* pTransferFunction1D, AbstrRenderer* requester) {
  for (Trans1DListIter i = m_vpTrans1DList.begin();i<m_vpTrans1DList.end();i++) {
    if (i->pTransferFunction1D == pTransferFunction1D) {
      for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
        if (*j == requester) {
          i->qpUser.erase(j);
          if (i->qpUser.size() == 0) {
            m_MasterController->DebugOut()->Message("GPUMemMan::Free1DTrans","Released TransferFunction1D");
            delete pTransferFunction1D;
            m_vpTrans1DList.erase(i);
          } else {
            m_MasterController->DebugOut()->Message("GPUMemMan::Free1DTrans","Decreased access count but TransferFunction1D is still in use by another subsystem");
          }
          return;
        }
      }
    }
  }
  m_MasterController->DebugOut()->Warning("GPUMemMan::Free1DTrans","TransferFunction1D not found or not being used by requester");
}


// ******************** 2D Trans

void GPUMemMan::Changed2DTrans(AbstrRenderer* requester, TransferFunction2D* pTransferFunction2D) {
  m_MasterController->DebugOut()->Message("GPUMemMan::Changed2DTrans","Sending change notification for 2D transfer function");

  for (Trans2DListIter i = m_vpTrans2DList.begin();i<m_vpTrans2DList.end();i++) {
    if (i->pTransferFunction2D == pTransferFunction2D) {
      for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
        if (*j != requester) (*j)->Changed2DTrans();
      }
    }
  }
}

void GPUMemMan::GetEmpty2DTrans(const VECTOR2<size_t>& iSize, AbstrRenderer* requester, TransferFunction2D** ppTransferFunction2D, GLTexture2D** tex) {
  m_MasterController->DebugOut()->Message("GPUMemMan::GetEmpty2DTrans","Creating new empty 2D transfer function");
  *ppTransferFunction2D = new TransferFunction2D(iSize);
  *tex = new GLTexture2D(GLuint(iSize.x), GLuint(iSize.y), GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

  m_vpTrans2DList.push_back(Trans2DListElem(*ppTransferFunction2D, *tex, requester));
}

void GPUMemMan::Get2DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction2D** ppTransferFunction2D, GLTexture2D** tex) {
  m_MasterController->DebugOut()->Message("GPUMemMan::Get2DTransFromFile","Loading 2D transfer function from file");
  *ppTransferFunction2D = new TransferFunction2D(strFilename);
  *tex = new GLTexture2D(GLuint((*ppTransferFunction2D)->GetSize().x), GLuint((*ppTransferFunction2D)->GetSize().y), GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

  m_vpTrans2DList.push_back(Trans2DListElem(*ppTransferFunction2D, *tex, requester));
}

GLTexture2D* GPUMemMan::Access2DTrans(TransferFunction2D* pTransferFunction2D, AbstrRenderer* requester) {
  for (Trans2DListIter i = m_vpTrans2DList.begin();i<m_vpTrans2DList.end();i++) {
    if (i->pTransferFunction2D == pTransferFunction2D) {
      m_MasterController->DebugOut()->Message("GPUMemMan::Access2DTrans","Accessing 2D transferfunction");
      i->qpUser.push_back(requester);
      return i->pTexture;
    }
  }

  m_MasterController->DebugOut()->Error("GPUMemMan::Access2DTrans","Unable to find 2D transferfunction");
  return NULL;
}

void GPUMemMan::Free2DTrans(TransferFunction2D* pTransferFunction2D, AbstrRenderer* requester) {
  for (Trans2DListIter i = m_vpTrans2DList.begin();i<m_vpTrans2DList.end();i++) {
    if (i->pTransferFunction2D == pTransferFunction2D) {
      for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
        if (*j == requester) {
          i->qpUser.erase(j);
          if (i->qpUser.size() == 0) {
            m_MasterController->DebugOut()->Message("GPUMemMan::Free2DTrans","Released TransferFunction2D");
            delete pTransferFunction2D;
            m_vpTrans2DList.erase(i);
          } else {
            m_MasterController->DebugOut()->Message("GPUMemMan::Free2DTrans","Decreased access count but TransferFunction2D is still in use by another subsystem");
          }
          return;
        }
      }
    }
  }
  m_MasterController->DebugOut()->Warning("GPUMemMan::Free2DTrans","TransferFunction2D not found or not being used by requester");
}

// ******************** 3D Textures

GLTexture3D* GPUMemMan::Get3DTexture(VolumeDataset* pDataset, const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) {
  for (Texture3DListIter i = m_vpTex3DList.begin();i<m_vpTex3DList.end();i++) {
    if ((*i)->Match(pDataset, vLOD, vBrick)) {
      m_MasterController->DebugOut()->Message("GPUMemMan::Get3DTexture","Reusing 3D texture");
      return (*i)->pTexture;
    }
  }

  // TODO: add code here to release textures/memory if we run out of free GPU/CPU mem

  m_MasterController->DebugOut()->Message("GPUMemMan::Get3DTexture","Creating new texture");

  Texture3DListElem* pNew3DTex = new Texture3DListElem(pDataset, vLOD, vBrick);

  m_iAllocatedGPUMemory += pNew3DTex->pTexture->GetCPUSize();
  m_iAllocatedCPUMemory += pNew3DTex->pTexture->GetGPUSize();

  m_vpTex3DList.push_back(pNew3DTex);
  return (*(m_vpTex3DList.end()-1))->pTexture;
}