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
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \date    August 2008
*/

#pragma once

#ifndef GPUMEMMAN_H
#define GPUMEMMAN_H

#include <deque>
#include <string>
#include <Renderer//AbstrRenderer.h>
#include <Renderer/GLTexture1D.h>
#include <Renderer/GLTexture2D.h>
#include <Renderer/GLTexture3D.h>
#include <IO/VolumeDataset.h>
#include <IO//TransferFunction1D.h>
#include <IO//TransferFunction2D.h>
#include <Basics/SystemInfo.h>

typedef std::deque< AbstrRenderer* > AbstrRendererList;
typedef AbstrRendererList::iterator AbstrRendererListIter;

// volume datasets
struct VolDataListElem {
  VolDataListElem(VolumeDataset* _pVolumeDataset, AbstrRenderer* pUser) :
    pVolumeDataset(_pVolumeDataset) 
  {
    qpUser.push_back(pUser);
  }

  VolumeDataset*  pVolumeDataset;
  AbstrRendererList qpUser;
};
typedef std::deque<VolDataListElem> VolDataList;
typedef VolDataList::iterator VolDataListIter;

// simple textures
struct SimpleTextureListElem {
  SimpleTextureListElem(unsigned int _iAccessCounter, GLTexture2D* _pTexture, std::string _strFilename) :
    iAccessCounter(_iAccessCounter), 
    pTexture(_pTexture), 
    strFilename(_strFilename)
  {}

  unsigned int  iAccessCounter;
  GLTexture2D*  pTexture;
  std::string    strFilename;
};
typedef std::deque<SimpleTextureListElem> SimpleTextureList;
typedef SimpleTextureList::iterator SimpleTextureListIter;

// 1D transfer functions
struct Trans1DListElem {
  Trans1DListElem(TransferFunction1D* _pTransferFunction1D, GLTexture1D* _pTexture, AbstrRenderer* pUser) :
    pTransferFunction1D(_pTransferFunction1D),
    pTexture(_pTexture)
  {
    qpUser.push_back(pUser);
  }

  TransferFunction1D*  pTransferFunction1D;
  GLTexture1D*    pTexture;
  AbstrRendererList  qpUser;
};
typedef std::deque<Trans1DListElem> Trans1DList;
typedef Trans1DList::iterator Trans1DListIter;

// 2D transfer functions
struct Trans2DListElem {
  Trans2DListElem(TransferFunction2D* _pTransferFunction2D, GLTexture2D* _pTexture, AbstrRenderer* pUser) :
    pTransferFunction2D(_pTransferFunction2D),
    pTexture(_pTexture)
  {
    qpUser.push_back(pUser);
  }

  TransferFunction2D*  pTransferFunction2D;
  GLTexture2D*    pTexture;
  AbstrRendererList  qpUser;
};
typedef std::deque<Trans2DListElem> Trans2DList;
typedef Trans2DList::iterator Trans2DListIter;


// 3D textures
struct Texture3DListElem {
  Texture3DListElem(VolumeDataset* _pDataset, const std::vector<UINT64>& _vLOD, const std::vector<UINT64>& _vBrick) :
    pData(NULL),
    pTexture(NULL),
    pDataset(_pDataset),
    vLOD(_vLOD),
    vBrick(_vBrick)
  {
     CreateTexture();
  }

  ~Texture3DListElem() {
      FreeData();
      FreeTexture();
  }

  bool Match(VolumeDataset* _pDataset, const std::vector<UINT64>& _vLOD, const std::vector<UINT64>& _vBrick) {
    if (_pDataset != pDataset || _vLOD.size() != vLOD.size() || _vBrick.size() != vBrick.size()) return false;

    for (size_t i = 0;i<vLOD.size();i++)   if (vLOD[i] != _vLOD[i]) return false;
    for (size_t i = 0;i<vBrick.size();i++) if (vBrick[i] != _vBrick[i]) return false;

    return true;
  }


  bool LoadData() {
    FreeData();
    return pDataset->GetBrick(&pData, vLOD, vBrick);
  }

  void  FreeData() {
    delete [] pData;
    pData = NULL;
  }

  bool CreateTexture() {
    FreeTexture();
    if (pData == NULL) {
      if (!LoadData()) return false;
    }

    const std::vector<UINT64> vSize = pDataset->GetInfo()->GetBrickSize(vLOD, vBrick);

    UINT64 iBitWidth  = pDataset->GetInfo()->GetBitwith();
    UINT64 iCompCount = pDataset->GetInfo()->GetComponentCount();

    GLint glInternalformat;
    GLenum glFormat;
    GLenum glType;

    switch (iCompCount) {
      case 1 : glFormat = GL_LUMINANCE; break;
      case 3 : glFormat = GL_RGB; break;
      case 4 : glFormat = GL_RGBA; break;
      default : return false;
    }

    if (iBitWidth == 8) {
        glType = GL_UNSIGNED_BYTE;
      
        switch (iCompCount) {
          case 1 : glInternalformat = GL_LUMINANCE8; break;
          case 3 : glInternalformat = GL_RGB8; break;
          case 4 : glInternalformat = GL_RGBA8; break;
          default : return false;
        }
    } else {
      if (iBitWidth == 16) {
        glType = GL_UNSIGNED_SHORT;
        switch (iCompCount) {
          case 1 : glInternalformat = GL_LUMINANCE16; break;
          case 3 : glInternalformat = GL_RGB16; break;
          case 4 : glInternalformat = GL_RGBA16; break;
          default : return false;
        }

      } else {
          return false;
      }
    }


    pTexture = new GLTexture3D(GLuint(vSize[0]), GLuint(vSize[1]), GLuint(vSize[2]), glInternalformat, glFormat, glType, pData);

    return true;
  }

  void  FreeTexture() {
    if (pTexture != NULL) {
      pTexture->Delete();
      delete pTexture;
    }
    pTexture = NULL;
  }


  unsigned char*      pData;
  GLTexture3D*        pTexture;
  VolumeDataset*      pDataset;
  std::vector<UINT64> vLOD;
  std::vector<UINT64> vBrick;
};
typedef std::deque<Texture3DListElem*> Texture3DList;
typedef Texture3DList::iterator Texture3DListIter;


class MasterController;

class GPUMemMan {
  public:
    GPUMemMan(MasterController* masterController);
    virtual ~GPUMemMan();

    VolumeDataset* LoadDataset(const std::string& strFilename, AbstrRenderer* requester);
    void FreeDataset(VolumeDataset* pVolumeDataset, AbstrRenderer* requester);

    void Changed1DTrans(AbstrRenderer* requester, TransferFunction1D* pTransferFunction1D);
    void GetEmpty1DTrans(size_t iSize, AbstrRenderer* requester, TransferFunction1D** ppTransferFunction1D, GLTexture1D** tex);
    void Get1DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction1D** ppTransferFunction1D, GLTexture1D** tex);
    GLTexture1D* Access1DTrans(TransferFunction1D* pTransferFunction1D, AbstrRenderer* requester);
    void Free1DTrans(TransferFunction1D* pTransferFunction1D, AbstrRenderer* requester);

    void Changed2DTrans(AbstrRenderer* requester, TransferFunction2D* pTransferFunction2D);
    void GetEmpty2DTrans(const VECTOR2<size_t>& iSize, AbstrRenderer* requester, TransferFunction2D** ppTransferFunction2D, GLTexture2D** tex);
    void Get2DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction2D** ppTransferFunction2D, GLTexture2D** tex);
    GLTexture2D* Access2DTrans(TransferFunction2D* pTransferFunction2D, AbstrRenderer* requester);
    void Free2DTrans(TransferFunction2D* pTransferFunction2D, AbstrRenderer* requester);

    GLTexture2D* Load2DTextureFromFile(const std::string& strFilename);
    void FreeTexture(GLTexture2D* pTexture);

    GLTexture3D* Get3DTexture(VolumeDataset* pDataset, const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick);

    // system statistics
    UINT64 GetCPUMem() const {return m_SystemInfo->m_iCPUMemSize;}
    UINT64 GetGPUMem() const {return m_SystemInfo->m_iGPUMemSize;}
    unsigned int GetBitWithMem() const {return m_SystemInfo->m_iProgrammBitWith;}
    unsigned int GetNumCPUs() const {return m_SystemInfo->m_iNumberofCPUs;}


  private:
    VolDataList       m_vpVolumeDatasets;
    SimpleTextureList m_vpSimpleTextures;
    Trans1DList       m_vpTrans1DList;
    Trans2DList       m_vpTrans2DList;
    Texture3DList     m_vpTex3DList;
    MasterController* m_MasterController;
    SystemInfo*       m_SystemInfo;
};

#endif // GPUMEMMAN_H
