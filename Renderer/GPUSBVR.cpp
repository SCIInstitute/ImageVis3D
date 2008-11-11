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
  \file    GPUSBVR.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/


#include "GPUSBVR.h"

#include <cmath>
#include <Basics/SysTools.h>
#include <Controller/MasterController.h>
#include <algorithm>
#include <ctime>

using namespace std;

GPUSBVR::GPUSBVR(MasterController* pMasterController) :
  GLRenderer(pMasterController),
  m_pFBO3DImageLast(NULL),
  m_pFBO3DImageCurrent(NULL),
  m_iFilledBuffers(0)
{
  m_pProgram1DTrans[0]   = NULL;
  m_pProgram1DTrans[1]   = NULL;
  m_pProgram2DTrans[0]   = NULL;
  m_pProgram2DTrans[1]   = NULL;
  m_pProgramIso          = NULL;
  m_pProgramTrans        = NULL;
  m_pProgram1DTransSlice = NULL;
  m_pProgram2DTransSlice = NULL;
}

GPUSBVR::~GPUSBVR() {
  delete [] m_p1DData;
  delete [] m_p2DData;
}


bool GPUSBVR::Initialize() {
  if (!GLRenderer::Initialize()) {
    m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","Error in parent call aborting");
    return false;
  }

  glShadeModel(GL_SMOOTH);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  
  m_IDTex[0] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin1.bmp").c_str());
  if (m_IDTex[0] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","First Image load failed");    
  }
  m_IDTex[1] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin2x2.bmp").c_str());
  if (m_IDTex[1] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","Second Image load failed");    
  }
  m_IDTex[2] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin1x3.bmp").c_str());
  if (m_IDTex[2] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","Third Image load failed");    
  }

  m_pProgram1DTrans[0] = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-VS.glsl"),
                                                                       SysTools::GetFromResourceOnMac("GPUSBVR-1D-FS.glsl"));
  
  m_pProgram1DTrans[1] = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-VS.glsl"),
                                                                       SysTools::GetFromResourceOnMac("GPUSBVR-1D-light-FS.glsl"));

  m_pProgram2DTrans[0] = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-VS.glsl"),
                                                                       SysTools::GetFromResourceOnMac("GPUSBVR-2D-FS.glsl"));

  m_pProgram2DTrans[1] = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-VS.glsl"),
                                                                       SysTools::GetFromResourceOnMac("GPUSBVR-2D-light-FS.glsl"));

  m_pProgramIso = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-VS.glsl"),
                                                                SysTools::GetFromResourceOnMac("GPUSBVR-ISO-FS.glsl"));

  m_pProgramTrans = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-Transfer-VS.glsl"),
                                                                SysTools::GetFromResourceOnMac("GPUSBVR-Transfer-FS.glsl"));

  m_pProgram1DTransSlice = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-Transfer-VS.glsl"),
                                                                SysTools::GetFromResourceOnMac("GPUSBVR-1D-slice-FS.glsl"));

  m_pProgram2DTransSlice = m_pMasterController->MemMan()->GetGLSLProgram(SysTools::GetFromResourceOnMac("GPUSBVR-Transfer-VS.glsl"),
                                                                SysTools::GetFromResourceOnMac("GPUSBVR-2D-slice-FS.glsl"));



  if (m_pProgram1DTrans[0] == NULL || m_pProgram1DTrans[1] == NULL ||
      m_pProgram2DTrans[0] == NULL || m_pProgram2DTrans[1] == NULL ||
      m_pProgramIso == NULL || m_pProgramTrans == NULL ||
      m_pProgram1DTransSlice == NULL || m_pProgram2DTransSlice == NULL) {
  
      m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","Error loading a shader.");
      return false;

  } else {

    m_pProgram1DTrans[0]->Enable();
    m_pProgram1DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[0]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[0]->Disable();

    m_pProgram1DTrans[1]->Enable();
    m_pProgram1DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[1]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram1DTrans[1]->Disable();

    m_pProgram2DTrans[0]->Enable();
    m_pProgram2DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[0]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[0]->Disable();

    m_pProgram2DTrans[1]->Enable();
    m_pProgram2DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[1]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram2DTrans[1]->Disable();

    m_pProgramIso->Enable();
    m_pProgramIso->SetUniformVector("texVolume",0);
    m_pProgramIso->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgramIso->SetUniformVector("vLightDiffuse",0.8f,0.8f,0.8f);
    m_pProgramIso->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgramIso->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgramIso->Disable();

    m_pProgramTrans->Enable();
    m_pProgramTrans->SetUniformVector("texColor",0);
    m_pProgramTrans->SetUniformVector("texDepth",1);
    m_pProgramTrans->Disable();

    m_pProgram1DTransSlice->Enable();
    m_pProgram1DTransSlice->SetUniformVector("texVolume",0);
    m_pProgram1DTransSlice->SetUniformVector("texTrans1D",1);
    m_pProgram1DTransSlice->Disable();

    m_pProgram2DTransSlice->Enable();
    m_pProgram2DTransSlice->SetUniformVector("texVolume",0);
    m_pProgram2DTransSlice->SetUniformVector("texTrans2D",1);
    m_pProgram2DTransSlice->Disable();
  }

  return true;
}

void GPUSBVR::SetBrickDepShaderVarsSlice(const UINTVECTOR3& vVoxelCount) {
  if (m_eRenderMode ==  RM_2DTRANS) {
    FLOATVECTOR3 vStep = 1.0f/FLOATVECTOR3(vVoxelCount);
    m_pProgram2DTransSlice->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
  }
}

void GPUSBVR::SetBrickDepShaderVars(const Brick& currentBrick) {

  FLOATVECTOR3 vStep(1.0f/currentBrick.vVoxelCount.x, 1.0f/currentBrick.vVoxelCount.y, 1.0f/currentBrick.vVoxelCount.z);

  float fStepScale = m_SBVRGeogen.GetOpacityCorrection();

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {                    
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            if (m_bUseLigthing)
                                m_pProgram1DTrans[1]->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            break;
                          }
    case RM_2DTRANS    :  {
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            break;
                          }
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::SetBrickDepShaderVars","Invalid rendermode set"); break;
  }

}

void GPUSBVR::SetDataDepShaderVars() {
  size_t       iMaxValue = m_p1DTrans->GetSize();
  unsigned int iMaxRange = (unsigned int)(1<<m_pDataset->GetInfo()->GetBitwith());
  float fScale = float(iMaxRange)/float(iMaxValue);

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fTransScale",fScale);
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable();

                            m_pProgram1DTransSlice->Enable();
                            m_pProgram1DTransSlice->SetUniformVector("fTransScale",fScale);
                            m_pProgram1DTransSlice->Disable();

                            break;
                          }
    case RM_2DTRANS    :  {
                            float fGradientScale = 1.0f/m_pDataset->GetMaxGradMagnitude();
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fTransScale",fScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fGradientScale",fGradientScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable();

                            m_pProgram2DTransSlice->Enable();
                            m_pProgram2DTransSlice->SetUniformVector("fTransScale",fScale);
                            m_pProgram2DTransSlice->SetUniformVector("fGradientScale",fGradientScale);
                            m_pProgram2DTransSlice->Disable();

                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->Enable();
                            m_pProgramIso->SetUniformVector("fIsoval",m_fIsovalue/fScale);
                            m_pProgramIso->Disable();

                            float fGradientScale = 1.0f/m_pDataset->GetMaxGradMagnitude();
                            m_pProgram2DTransSlice->Enable();
                            m_pProgram2DTransSlice->SetUniformVector("fTransScale",fScale);
                            m_pProgram2DTransSlice->SetUniformVector("fGradientScale",fGradientScale);
                            m_pProgram2DTransSlice->Disable();

                            break;
                          }
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::SetDataDepShaderVars","Invalid rendermode set"); break;
  }


}

bool GPUSBVR::LoadDataset(const string& strFilename) {
  if (GLRenderer::LoadDataset(strFilename)) {
    if (m_pProgram1DTrans[0] != NULL) SetDataDepShaderVars();
    return true;
  } else return false;
}


void GPUSBVR::DrawLogo() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-0.5, +0.5, +0.5, -0.5, 0.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  if (m_IDTex[size_t(m_eViewMode)] != NULL) m_IDTex[size_t(m_eViewMode)]->Bind();
  glDisable(GL_TEXTURE_3D);
  glEnable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
    glColor4d(1,1,1,1);
    glTexCoord2d(0,0);
    glVertex3d(0.2, 0.4, -0.5);
    glTexCoord2d(1,0);
    glVertex3d(0.4, 0.4, -0.5);
    glTexCoord2d(1,1);
    glVertex3d(0.4, 0.2, -0.5);
    glTexCoord2d(0,1);
    glVertex3d(0.2, 0.2, -0.5);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void GPUSBVR::DrawBackGradient() {
  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-0.5, +0.5, +0.5, -0.5, 0.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
    glColor4d(m_vBackgroundColors[1].x,m_vBackgroundColors[1].y,m_vBackgroundColors[1].z,1);
    glVertex3d(-1.0,  1.0, -0.5);
    glVertex3d( 1.0,  1.0, -0.5);
    glColor4d(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,1);
    glVertex3d( 1.0, -1.0, -0.5);
    glVertex3d(-1.0, -1.0, -0.5);
  glEnd();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glEnable(GL_DEPTH_TEST);
}


bool GPUSBVR::Render2DView(ERenderArea eREnderArea, EWindowMode eDirection, UINT64 iSliceIndex) {
  // clear the depth buffer if instructed
  if (m_bClearFramebuffer) glClear(GL_DEPTH_BUFFER_BIT);  
   switch (m_eRenderMode) {
    case RM_2DTRANS    :  m_p2DTransTex->Bind(1); 
                          m_pProgram2DTransSlice->Enable();
                          break;
    default            :  m_p1DTransTex->Bind(1); 
                          m_pProgram1DTransSlice->Enable();
                          break;
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  UINT64 iCurrentLOD = 0;
  UINTVECTOR3 vVoxelCount;

  /// \todo change this code to use something better than the biggest single brick LOD level
  for (UINT64 i = 0;i<m_pDataset->GetInfo()->GetLODLevelCount();i++) {
    if (m_pDataset->GetInfo()->GetBrickCount(i).volume() == 1) {
        iCurrentLOD = i;
        vVoxelCount = UINTVECTOR3(m_pDataset->GetInfo()->GetDomainSize(i));
    }
  }

  SetBrickDepShaderVarsSlice(vVoxelCount);

  // convert 3D variables to the more general ND scheme used in the memory manager, e.i. convert 3-vectors to stl vectors
  vector<UINT64> vLOD; vLOD.push_back(iCurrentLOD);
  vector<UINT64> vBrick; 
  vBrick.push_back(0);vBrick.push_back(0);vBrick.push_back(0);

  // get the 3D texture from the memory manager
  GLTexture3D* t = m_pMasterController->MemMan()->Get3DTexture(m_pDataset, vLOD, vBrick, 0, m_iFrameCounter);
  if(t!=NULL) t->Bind(0);

  // clear the target at the beginning
  SetRenderTargetAreaScissor(eREnderArea);
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);

  FLOATVECTOR3 vMinCoords(0.5f/FLOATVECTOR3(vVoxelCount));
  FLOATVECTOR3 vMaxCoords(1.0f-vMinCoords);

  UINT64VECTOR3 vDomainSize = m_pDataset->GetInfo()->GetDomainSize();
  DOUBLEVECTOR3 vAspectRatio = m_pDataset->GetInfo()->GetScale() * DOUBLEVECTOR3(vDomainSize);  

  DOUBLEVECTOR2 vWinAspectRatio = 1.0 / DOUBLEVECTOR2(m_vWinSize);
  vWinAspectRatio = vWinAspectRatio / vWinAspectRatio.maxVal();

  switch (eDirection) {
    case WM_CORONAL : {
                          DOUBLEVECTOR2 v2AspectRatio = vAspectRatio.xz()*DOUBLEVECTOR2(vWinAspectRatio);
                          v2AspectRatio = v2AspectRatio / v2AspectRatio.maxVal();
                          double fSliceIndex = double(iSliceIndex)/double(vDomainSize.y);
                          glBegin(GL_QUADS);
                            glTexCoord3d(vMinCoords.x,fSliceIndex,vMaxCoords.z);
                            glVertex3d(-1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMaxCoords.x,fSliceIndex,vMaxCoords.z);
                            glVertex3d(+1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMaxCoords.x,fSliceIndex,vMinCoords.z);
                            glVertex3d(+1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMinCoords.x,fSliceIndex,vMinCoords.z);
                            glVertex3d(-1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                          glEnd();
                          break;
                      }
    case WM_AXIAL : {
                          DOUBLEVECTOR2 v2AspectRatio = vAspectRatio.xy()*DOUBLEVECTOR2(vWinAspectRatio);
                          v2AspectRatio = v2AspectRatio / v2AspectRatio.maxVal();
                          double fSliceIndex = double(iSliceIndex)/double(vDomainSize.z);
                          glBegin(GL_QUADS);
                            glTexCoord3d(vMinCoords.x,vMaxCoords.y,fSliceIndex);
                            glVertex3d(-1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMaxCoords.x,vMaxCoords.y,fSliceIndex);
                            glVertex3d(+1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMaxCoords.x,vMinCoords.y,fSliceIndex);
                            glVertex3d(+1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(vMinCoords.x,vMinCoords.y,fSliceIndex);
                            glVertex3d(-1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                          glEnd();
                          break;
                      }
    case WM_SAGITTAL : {
                          DOUBLEVECTOR2 v2AspectRatio = vAspectRatio.yz()*DOUBLEVECTOR2(vWinAspectRatio);
                          v2AspectRatio = v2AspectRatio / v2AspectRatio.maxVal();
                          double fSliceIndex = double(iSliceIndex)/double(vDomainSize.x);
                          glBegin(GL_QUADS);
                            glTexCoord3d(fSliceIndex,vMinCoords.y,vMaxCoords.z);
                            glVertex3d(-1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(fSliceIndex,vMaxCoords.y,vMaxCoords.z);
                            glVertex3d(+1.0f*v2AspectRatio.x, +1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(fSliceIndex,vMaxCoords.y,vMinCoords.z);
                            glVertex3d(+1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                            glTexCoord3d(fSliceIndex,vMinCoords.y,vMinCoords.z);
                            glVertex3d(-1.0f*v2AspectRatio.x, -1.0f*v2AspectRatio.y, -0.5f);
                          glEnd();
                          break;
                      }
    default        :  m_pMasterController->DebugOut()->Error("GPUSBVR::Render2DView","Invalid windowmode set"); break;
  }

  m_pMasterController->MemMan()->Release3DTexture(t);

  glEnable(GL_DEPTH_TEST);

  switch (m_eRenderMode) {
    case RM_2DTRANS    :  m_pProgram2DTransSlice->Disable(); break;
    default            :  m_pProgram1DTransSlice->Disable(); break;
  }

  return true;
}

void GPUSBVR::RenderBBox(const FLOATVECTOR4 vColor) {
  UINT64VECTOR3 vDomainSize = m_pDataset->GetInfo()->GetDomainSize();
  UINT64 iMaxDomainSize = vDomainSize.maxVal();
  FLOATVECTOR3 vExtend = FLOATVECTOR3(vDomainSize)/float(iMaxDomainSize) * FLOATVECTOR3(m_pDataset->GetInfo()->GetScale());

  FLOATVECTOR3 vCenter(0,0,0);
  RenderBBox(vColor, vCenter, vExtend);
}

void GPUSBVR::RenderBBox(const FLOATVECTOR4 vColor, const FLOATVECTOR3& vCenter, const FLOATVECTOR3& vExtend) {
  FLOATVECTOR3 vMinPoint, vMaxPoint;
  vMinPoint = (vCenter - vExtend/2.0);
  vMaxPoint = (vCenter + vExtend/2.0);

  glBegin(GL_LINES);
    glColor4f(vColor.x,vColor.y,vColor.z,vColor.w);
    // FRONT
    glVertex3f( vMaxPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f(vMinPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y,vMinPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y,vMinPoint.z);
    glVertex3f(vMinPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y,vMinPoint.z);
    glVertex3f( vMaxPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y,vMinPoint.z);

    // BACK
    glVertex3f( vMaxPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glVertex3f( vMaxPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);

    // CONNECTION
    glVertex3f(vMinPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glVertex3f(vMinPoint.x, vMaxPoint.y,vMinPoint.z);
    glVertex3f( vMaxPoint.x,vMinPoint.y, vMaxPoint.z);
    glVertex3f( vMaxPoint.x,vMinPoint.y,vMinPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y,vMinPoint.z);
  glEnd();

}


vector<Brick> GPUSBVR::BuildFrameBrickList() {
  vector<Brick> vBrickList;

  UINT64VECTOR3 vOverlap = m_pDataset->GetInfo()->GetBrickOverlapSize();
  UINT64VECTOR3 vBrickDimension = m_pDataset->GetInfo()->GetBrickCount(m_iCurrentLOD);
  UINT64VECTOR3 vDomainSize = m_pDataset->GetInfo()->GetDomainSize(m_iCurrentLOD);
  UINT64 iMaxDomainSize = vDomainSize.maxVal();
  FLOATVECTOR3 vScale(m_pDataset->GetInfo()->GetScale().x, 
                      m_pDataset->GetInfo()->GetScale().y, 
                      m_pDataset->GetInfo()->GetScale().z);


  FLOATVECTOR3 vDomainExtend = FLOATVECTOR3(vScale.x*vDomainSize.x, vScale.y*vDomainSize.y, vScale.z*vDomainSize.z) / iMaxDomainSize;


  FLOATVECTOR3 vBrickCorner;

  for (UINT64 z = 0;z<vBrickDimension.z;z++) {
    Brick b;
    for (UINT64 y = 0;y<vBrickDimension.y;y++) {
      for (UINT64 x = 0;x<vBrickDimension.x;x++) {
        
        UINT64VECTOR3 vSize = m_pDataset->GetInfo()->GetBrickSize(m_iCurrentLOD, UINT64VECTOR3(x,y,z));
        b = Brick(x,y,z, (unsigned int)(vSize.x), (unsigned int)(vSize.y), (unsigned int)(vSize.z));


        FLOATVECTOR3 vEffectiveSize = m_pDataset->GetInfo()->GetEffectiveBrickSize(m_iCurrentLOD, UINT64VECTOR3(x,y,z));


        b.vExtension.x = float(vEffectiveSize.x/float(iMaxDomainSize) * vScale.x);
        b.vExtension.y = float(vEffectiveSize.y/float(iMaxDomainSize) * vScale.y);
        b.vExtension.z = float(vEffectiveSize.z/float(iMaxDomainSize) * vScale.z);
        
        // compute center of the brick
        b.vCenter = (vBrickCorner + b.vExtension/2.0f)-vDomainExtend*0.5f;

        vBrickCorner.x += b.vExtension.x;


        // if the brick is visible (i.e. inside the frustum) continue processing
        if (m_FrustumCullingLOD.IsVisible(b.vCenter, b.vExtension)) {

          // compute 
          b.vTexcoordsMin = FLOATVECTOR3((x == 0) ? 0.5f/b.vVoxelCount.x : vOverlap.x*0.5f/b.vVoxelCount.x,
                                         (y == 0) ? 0.5f/b.vVoxelCount.y : vOverlap.y*0.5f/b.vVoxelCount.y,
                                         (z == 0) ? 0.5f/b.vVoxelCount.z : vOverlap.z*0.5f/b.vVoxelCount.z);
          b.vTexcoordsMax = FLOATVECTOR3((x == vBrickDimension.x-1) ? 1.0f-0.5f/b.vVoxelCount.x : 1.0f-vOverlap.x*0.5f/b.vVoxelCount.x,
                                         (y == vBrickDimension.y-1) ? 1.0f-0.5f/b.vVoxelCount.y : 1.0f-vOverlap.y*0.5f/b.vVoxelCount.y,
                                         (z == vBrickDimension.z-1) ? 1.0f-0.5f/b.vVoxelCount.z : 1.0f-vOverlap.z*0.5f/b.vVoxelCount.z);
          
          /// \todo change this to a more accurate distance compuation
          b.fDistance = (FLOATVECTOR4(b.vCenter,1.0f)*m_matModelView).xyz().length();

          // add the brick to the list of active bricks
          vBrickList.push_back(b);
        }
      }

      vBrickCorner.x  = 0;
      vBrickCorner.y += b.vExtension.y;
    }
    vBrickCorner.y = 0;
    vBrickCorner.z += b.vExtension.z;
  }

  // depth sort bricks
  sort(vBrickList.begin(), vBrickList.end());

  return vBrickList;
}

void GPUSBVR::Render3DView() {
  // ************** GL States ***********
  // Modelview
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  m_matModelView.setModelview();

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_2D);

  if (m_iBricksRenderedInThisSubFrame == 0) {
    // for rendering modes other than isosurface render the bbox in the first pass once to init the depth buffer
    // for isosurface rendering we can go ahead and render the bbox directly as isosurfacing 
    // writes out correct depth values
    glDisable(GL_BLEND);
    if (m_eRenderMode != RM_ISOSURFACE) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    if (m_bRenderGlobalBBox) RenderBBox();
    if (m_bRenderLocalBBox) {
      for (UINT64 iCurrentBrick = 0;iCurrentBrick<m_vCurrentBrickList.size();iCurrentBrick++) {
        RenderBBox(FLOATVECTOR4(0,1,0,1), m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension);
      }
    }
    if (m_eRenderMode != RM_ISOSURFACE) glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  }

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_p1DTransTex->Bind(1); 
                          m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                          glEnable(GL_BLEND);
                          glBlendEquation(GL_FUNC_ADD);
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_2DTRANS    :  m_p2DTransTex->Bind(1);
                          m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Enable(); 
                          glEnable(GL_BLEND);
                          glBlendEquation(GL_FUNC_ADD);
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_ISOSURFACE :  m_pProgramIso->Enable(); 
                          break;
    default    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Render3DView","Invalid rendermode set"); 
                          break;
  }

  if (m_eRenderMode != RM_ISOSURFACE) glDepthMask(GL_FALSE);

  // loop over all bricks in the current LOD level
  clock_t timeStart, timeProbe;
  timeStart = timeProbe = clock();

  while (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame && float(timeProbe-timeStart)*1000.0f/float(CLOCKS_PER_SEC) < m_iTimeSliceMSecs) {
  
    // setup the slice generator
    m_SBVRGeogen.SetVolumeData(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vExtension, m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vVoxelCount, 
                               m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vTexcoordsMin, m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vTexcoordsMax);
    FLOATMATRIX4 maBricktTrans; 
    maBricktTrans.Translation(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCenter.x, m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCenter.y, m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCenter.z);
    FLOATMATRIX4 maBricktModelView = maBricktTrans * m_matModelView;
    maBricktModelView.setModelview();
    m_SBVRGeogen.SetTransformation(maBricktModelView);

    // convert 3D variables to the more general ND scheme used in the memory manager, e.i. convert 3-vectors to stl vectors
    vector<UINT64> vLOD; vLOD.push_back(m_iCurrentLOD);
    vector<UINT64> vBrick; 
    vBrick.push_back(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCoords.x);
    vBrick.push_back(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCoords.y);
    vBrick.push_back(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame].vCoords.z);

    // get the 3D texture from the memory manager
    GLTexture3D* t = m_pMasterController->MemMan()->Get3DTexture(m_pDataset, vLOD, vBrick, m_iIntraFrameCounter++, m_iFrameCounter);
    if(t!=NULL) t->Bind(0);

    // update the shader parameter
    SetBrickDepShaderVars(m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame]);

    // render the slices
    glBegin(GL_TRIANGLES);
      for (int i = int(m_SBVRGeogen.m_vSliceTriangles.size())-1;i>=0;i--) {
        glTexCoord3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vTex);
        glVertex3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vPos);
      }
    glEnd();

    // release the 3D texture
    m_pMasterController->MemMan()->Release3DTexture(t);

    // count the bricks rendered
    m_iBricksRenderedInThisSubFrame++;
    timeProbe = clock();
  }

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
    case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
    case RM_ISOSURFACE :  m_pProgramIso->Disable(); break;
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Render3DView","Invalid rendermode set"); break;
  }

  // at the very end render the global bbox
  if (m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame) {
    if (m_eRenderMode != RM_ISOSURFACE) {    
      m_matModelView.setModelview();
      if (m_bRenderGlobalBBox) {
        glDisable(GL_DEPTH_TEST);
        RenderBBox();
      }

      if (m_bRenderLocalBBox) {
        glDisable(GL_DEPTH_TEST);
        for (UINT64 iCurrentBrick = 0;iCurrentBrick<m_vCurrentBrickList.size();iCurrentBrick++) {
          RenderBBox(FLOATVECTOR4(0,1,0,1), m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension);
        }
      }
      glDepthMask(GL_TRUE);
    }
  }

  glDisable(GL_BLEND);

}


bool GPUSBVR::CheckForRedraw() {
  if (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame || m_iCurrentLODOffset > m_iMinLODForCurrentView) {
    if (m_iCheckCounter == 0)  {
      m_pMasterController->DebugOut()->Message("GPUSBVR::CheckForRedraw","Still drawing...");
      return true;
    } else m_iCheckCounter--;
  }
  return m_bPerformRedraw;
}


void GPUSBVR::Plan3DFrame() {
  if (m_bPerformRedraw) {
    // compute modelviewmatrix and pass it to the culling object
    m_matModelView = m_mRotation*m_mTranslation;
    m_FrustumCullingLOD.SetViewMatrix(m_matModelView);
    m_FrustumCullingLOD.Update();

    m_iCurrentLODOffset = m_iMaxLODIndex+1;
    ComputeMinLODForCurrentView();
  }

  // plan if the frame is to be redrawn
  // or if we have completed the last subframe but not the entire frame
  if (m_bPerformRedraw || 
     (m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame && m_iCurrentLODOffset > m_iMinLODForCurrentView)) {

    // compute current LOD level
    m_iCurrentLODOffset--;
    m_iCurrentLOD = std::min<UINT64>(m_iCurrentLODOffset,m_pDataset->GetInfo()->GetLODLevelCount()-1);
    UINT64VECTOR3 vBrickCount = m_pDataset->GetInfo()->GetBrickCount(m_iCurrentLOD);

    // build new brick todo-list
    m_vCurrentBrickList = BuildFrameBrickList();
    m_iBricksRenderedInThisSubFrame = 0;
  }

  if (m_bPerformRedraw) {
    // update frame states
    m_iIntraFrameCounter = 0;
    m_iFrameCounter = m_pMasterController->MemMan()->UpdateFrameCounter();
  }
}


bool GPUSBVR::Execute3DFrame(ERenderArea eREnderArea) {
  // are we starting a new LOD level?
  if (m_iBricksRenderedInThisSubFrame == 0) m_iFilledBuffers = 0;

  // if there is something left in the TODO list
  if (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame) {
    // setup shaders vars
    SetDataDepShaderVars(); 

    // clear target at the beginning
    if (m_iBricksRenderedInThisSubFrame == 0) {
      SetRenderTargetAreaScissor(eREnderArea);
      glClearColor(0,0,0,0);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      glDisable( GL_SCISSOR_TEST );
    }

    Render3DView();

    // if there is nothing left todo in this subframe -> present the result
    if (m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame) return true;
  } 
  return false;
}



void GPUSBVR::Paint() {
  // if we are redrawing clear the framebuffer (if requested)
  if (m_bPerformRedraw && m_bClearFramebuffer) glClear(GL_DEPTH_BUFFER_BIT);

  bool bNewDataToShow;
  if (m_eViewMode == VM_SINGLE) {
    // set render area to fullscreen
    SetRenderTargetArea(RA_FULLSCREEN);

    // bind offscreen buffer
    m_pFBO3DImageCurrent->Write();

    switch (m_eFullWindowMode) {
       case WM_3D       : {
                              // plan the frame
                              Plan3DFrame();
                              // execute the frame
                              bNewDataToShow = Execute3DFrame(RA_FULLSCREEN); 
                              break;
                          }
       case WM_SAGITTAL : 
       case WM_AXIAL    : 
       case WM_CORONAL  : bNewDataToShow = Render2DView(RA_FULLSCREEN, m_eFullWindowMode, m_piSlice[size_t(m_eFullWindowMode)]); break;
       default          : m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid Windowmode");
                          bNewDataToShow = false;
                          break;

    }

    // unbind offscreen buffer
    m_pFBO3DImageCurrent->FinishWrite();

  } else { // VM_TWOBYTWO 
    int iActiveRenderWindows = 0;
    int iReadyWindows = 0;
    for (unsigned int i = 0;i<4;i++) if (m_bRedrawMask[i]) iActiveRenderWindows++;

    // bind offscreen buffer
    m_pFBO3DImageCurrent->Write();

    for (unsigned int i = 0;i<4;i++) {
      ERenderArea eArea = ERenderArea(int(RA_TOPLEFT)+i);

      if (m_bRedrawMask[size_t(m_e2x2WindowMode[i])]) {
        SetRenderTargetArea(eArea);
        bool bLocalNewDataToShow;
        switch (m_e2x2WindowMode[i]) {
           case WM_3D       : {
                                // plan the frame
                                Plan3DFrame();
                                // execute the frame
                                bLocalNewDataToShow = Execute3DFrame(eArea);
                                // are we done traversing the LOD levels
                                m_bRedrawMask[size_t(m_e2x2WindowMode[i])] = (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame) || (m_iCurrentLODOffset > m_iMinLODForCurrentView);
                                break;
                              }
           case WM_SAGITTAL : 
           case WM_AXIAL    : 
           case WM_CORONAL  : bLocalNewDataToShow= Render2DView(eArea, m_e2x2WindowMode[i], m_piSlice[size_t(m_e2x2WindowMode[i])]); 
                              m_bRedrawMask[size_t(m_e2x2WindowMode[i])] = false;
                              break;
           default          : m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid Windowmode");
                              bLocalNewDataToShow = false; 
                              break;
        }
        
        if (bLocalNewDataToShow) iReadyWindows++;
      } else {
        SetRenderTargetArea(RA_FULLSCREEN);
        SetRenderTargetAreaScissor(eArea);
        RerenderPreviousResult(false);
      }

      bNewDataToShow = (iActiveRenderWindows > 0) && (iReadyWindows==iActiveRenderWindows);
    }


    // set render area to fullscreen
    SetRenderTargetAreaScissor(RA_FULLSCREEN);
    SetRenderTargetArea(RA_FULLSCREEN);
    glDisable( GL_SCISSOR_TEST );

    // render seperating lines
    glDisable(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, 1, -1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_LINES);
      glColor4f(1.0f,1.0f,1.0f,1.0f);
      glVertex3f(0,-1,0);
      glVertex3f(0,1,0);
      glVertex3f(-1,0,0);
      glVertex3f(1,0,0);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);

    // unbind offscreen buffer
    m_pFBO3DImageCurrent->FinishWrite();
  }
  // if the image is complete swap the offscreen buffers
  if (bNewDataToShow) swap(m_pFBO3DImageLast, m_pFBO3DImageCurrent);

  // show the result
  if (bNewDataToShow || m_iFilledBuffers < 2) RerenderPreviousResult(true);

  m_bPerformRedraw = false;
}

void GPUSBVR::Resize(const UINTVECTOR2& vWinSize) {
  AbstrRenderer::Resize(vWinSize);
  m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","Resizing to %i x %i", vWinSize.x, vWinSize.y);
  CreateOffscreenBuffer();
}

void GPUSBVR::SetRenderTargetArea(ERenderArea eREnderArea) {
  switch (eREnderArea) {
    case RA_TOPLEFT     : SetViewPort(UINTVECTOR2(0,m_vWinSize.y/2), UINTVECTOR2(m_vWinSize.x/2,m_vWinSize.y)); break;
    case RA_TOPRIGHT    : SetViewPort(m_vWinSize/2, m_vWinSize); break;
    case RA_LOWERLEFT   : SetViewPort(UINTVECTOR2(0,0),m_vWinSize/2); break;
    case RA_LOWERRIGHT  : SetViewPort(UINTVECTOR2(m_vWinSize.x/2,0), UINTVECTOR2(m_vWinSize.x,m_vWinSize.y/2)); break;
    case RA_FULLSCREEN  : SetViewPort(UINTVECTOR2(0,0), m_vWinSize); break;
    default             : m_pMasterController->DebugOut()->Error("GPUSBVR::SetRenderTargetArea","Invalid render area set"); break;
  }
}

void GPUSBVR::SetRenderTargetAreaScissor(ERenderArea eREnderArea) {
  switch (eREnderArea) {
    case RA_TOPLEFT     : glScissor(0,m_vWinSize.y/2, m_vWinSize.x/2,m_vWinSize.y); break;
    case RA_TOPRIGHT    : glScissor(m_vWinSize.x/2, m_vWinSize.y/2, m_vWinSize.x, m_vWinSize.y); break;
    case RA_LOWERLEFT   : glScissor(0,0,m_vWinSize.x/2, m_vWinSize.y/2); break;
    case RA_LOWERRIGHT  : glScissor(m_vWinSize.x/2,0,m_vWinSize.x,m_vWinSize.y/2); break;
    case RA_FULLSCREEN  : glScissor(0,0,m_vWinSize.x, m_vWinSize.y); break;
    default             : m_pMasterController->DebugOut()->Error("GPUSBVR::SetRenderTargetAreaScissor","Invalid render area set"); break;
  }
  glEnable( GL_SCISSOR_TEST );
}

void GPUSBVR::SetViewPort(UINTVECTOR2 viLowerLeft, UINTVECTOR2 viUpperRight) {

  UINTVECTOR2 viSize = viUpperRight-viLowerLeft;

  float aspect=(float)viSize.x/(float)viSize.y;
  float fovy = 50.0f;
  float nearPlane = 0.1f;
  float farPlane = 100.0f;
	glViewport(viLowerLeft.x,viLowerLeft.y,viSize.x,viSize.y);
	glMatrixMode(GL_PROJECTION);		
	glLoadIdentity();
	gluPerspective(fovy,aspect,nearPlane,farPlane); 	// Set Projection. Arguments are FOV (in degrees), aspect-ratio, near-plane, far-plane.
  
  // forward the GL projection matrix to the culling object
  FLOATMATRIX4 mProjection;
  mProjection.getProjection();
  m_FrustumCullingLOD.SetProjectionMatrix(mProjection);
  m_FrustumCullingLOD.SetScreenParams(fovy,aspect,nearPlane,viSize.y);

	glMatrixMode(GL_MODELVIEW);
}


void GPUSBVR::CreateOffscreenBuffer() {
  if (m_pFBO3DImageLast != NULL) m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImageLast);
  if (m_pFBO3DImageCurrent != NULL) m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImageCurrent);

  if (m_vWinSize.area() > 0) {
    switch (m_eBlendPrecision) {
      case BP_8BIT  : m_pFBO3DImageLast = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA8, 8*4, true);
                      m_pFBO3DImageCurrent = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA8, 8*4, true);
                      break;
      case BP_16BIT : m_pFBO3DImageLast = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, true);
                      m_pFBO3DImageCurrent = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, true);
                      break;
      case BP_32BIT : m_pFBO3DImageLast = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA32F_ARB, 32*4, true);
                      m_pFBO3DImageCurrent = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA32F_ARB, 32*4, true);
                      break;
      default       : m_pMasterController->DebugOut()->Message("GPUSBVR::CreateOffscreenBuffer","Invalid Blending Precision");
                      m_pFBO3DImageLast = NULL; m_pFBO3DImageCurrent = NULL;
                      break;
    }
  }
}

void GPUSBVR::SetBlendPrecision(EBlendPrecision eBlendPrecision) {
  if (eBlendPrecision != m_eBlendPrecision) {
    AbstrRenderer::SetBlendPrecision(eBlendPrecision);
    CreateOffscreenBuffer();
  }
}

void GPUSBVR::Cleanup() {
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[0]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[1]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[2]);

  m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImageLast);
  m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImageCurrent);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram1DTrans[0]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram1DTrans[1]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram2DTrans[0]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram2DTrans[1]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramIso);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramTrans);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram1DTransSlice);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram2DTransSlice);
}


void GPUSBVR::RerenderPreviousResult(bool bTransferToFramebuffer) {
  // clear the framebuffer
  if (m_bClearFramebuffer) {
    glDepthMask(GL_FALSE);
    if (m_vBackgroundColors[0] == m_vBackgroundColors[1]) {
      glClearColor(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,0);
      glClear(GL_COLOR_BUFFER_BIT); 
    } else DrawBackGradient();
    //DrawLogo();
    glDepthMask(GL_TRUE);
  }

  if (bTransferToFramebuffer) {
    glViewport(0,0,m_vWinSize.x,m_vWinSize.y);
    m_iFilledBuffers++;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }


  m_pFBO3DImageLast->Read(GL_TEXTURE0);
  m_pFBO3DImageLast->ReadDepth(GL_TEXTURE1);

  m_pProgramTrans->Enable();

  glDisable(GL_DEPTH_TEST);

  glBegin(GL_QUADS);
    glColor4d(1,1,1,1);
    glTexCoord2d(0,1);
    glVertex3d(-1.0,  1.0, -0.5);
    glTexCoord2d(1,1);
    glVertex3d( 1.0,  1.0, -0.5);
    glTexCoord2d(1,0);
    glVertex3d( 1.0, -1.0, -0.5);
    glTexCoord2d(0,0);
    glVertex3d(-1.0, -1.0, -0.5);
  glEnd();

  m_pProgramTrans->Disable();

  m_pFBO3DImageLast->FinishRead();
  m_pFBO3DImageLast->FinishDepthRead();
  glEnable(GL_DEPTH_TEST);
}


void GPUSBVR::SetSampleRateModifier(float fSampleRateModifier) {
  GLRenderer::SetSampleRateModifier(fSampleRateModifier);
  m_SBVRGeogen.SetSamplingModifier(fSampleRateModifier);
}