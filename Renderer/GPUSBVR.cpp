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
  m_iCurrentLOD(0),
  m_iBricksRenderedInThisSubFrame(0)
{
  m_pProgram1DTrans[0] = NULL;
  m_pProgram1DTrans[1] = NULL;
  m_pProgram2DTrans[0] = NULL;
  m_pProgram2DTrans[1] = NULL;
  m_pProgramIso        = NULL;
  m_pProgramTrans      = NULL;
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


  if (m_pProgram1DTrans[0] == NULL || m_pProgram1DTrans[1] == NULL ||
      m_pProgram2DTrans[0] == NULL || m_pProgram2DTrans[1] == NULL ||
      m_pProgramIso == NULL || m_pProgramTrans == NULL) {
  
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
  }

  return true;
}

void GPUSBVR::SetBrickDepShaderVars(UINT64 iCurrentLOD, const Brick& currentBrick) {

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
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::SetDataDepShaderVars","Invalid rendermode set"); break;
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
                            break;
                          }
    case RM_2DTRANS    :  {
                            float fGradientScale = 1.0f/m_pDataset->GetMaxGradMagnitude();
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fTransScale",fScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fGradientScale",fGradientScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable();
                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->Enable();
                            m_pProgramIso->SetUniformVector("fIsoval",m_fIsovalue/fScale);
                            m_pProgramIso->Disable();
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

void GPUSBVR::RenderSingle() {
  SetRenderTargetArea(RA_FULLSCREEN);
  switch (m_eWindowMode[0]) {
    case WM_3D         :  Render3DView(); break;
    case WM_CORONAL    :  
    case WM_AXIAL      :  
    case WM_SAGITTAL   :  Render2DView(m_eWindowMode[0], m_fSliceIndex[0]); break; 
    case WM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::RenderSingle","Invalid windowmode set"); break;
  }
}

void GPUSBVR::Render2by2() {

}


void GPUSBVR::Render2DView(EWindowMode eDirection, float fSliceIndex) {

}

void GPUSBVR::RenderBBox(const FLOATVECTOR4 vColor) {
  FLOATVECTOR3 vScale(m_pDataset->GetInfo()->GetScale().x, m_pDataset->GetInfo()->GetScale().y, m_pDataset->GetInfo()->GetScale().z);
  UINT64VECTOR3 vDomainSize = m_pDataset->GetInfo()->GetDomainSize(0);
  vScale.x *= float(vDomainSize.x)/float(vDomainSize.maxVal());
  vScale.y *= float(vDomainSize.y)/float(vDomainSize.maxVal());
  vScale.z *= float(vDomainSize.z)/float(vDomainSize.maxVal());

  FLOATVECTOR3 vCenter(0,0,0);

  RenderBBox(vColor, vCenter, vScale);
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
        if (m_FrustumCulling.IsVisible(b.vCenter, b.vExtension)) {

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
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Render3DView","Invalid rendermode set"); 
                          break;
  }

  if (m_eRenderMode != RM_ISOSURFACE) glDepthMask(GL_FALSE);

  // loop over all bricks in the current LOD level
  clock_t timeStart, timeProbe;
  timeStart = timeProbe = clock();

  unsigned int iBricksRenderedInThisBatch = 0;
  while (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame && float(timeProbe-timeStart)/float(CLOCKS_PER_SEC) < 1) {
  
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
    SetBrickDepShaderVars(m_iCurrentLOD, m_vCurrentBrickList[m_iBricksRenderedInThisSubFrame]);

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

UINT64 GPUSBVR::GetMinLODForCurrentView() {
  /// \todo - determine minimal LOD
  return 0;
} 

bool GPUSBVR::CheckForRedraw() {
  if (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame || m_iCurrentLODOffset > GetMinLODForCurrentView()) {
    if (m_iCheckCounter == 0)  {
      m_pMasterController->DebugOut()->Message("GPUSBVR::CheckForRedraw","Continuing to draw");
      return true;
    } else m_iCheckCounter--;
  }
  return m_bCompleteRedraw;
}


void GPUSBVR::PlanFrame() {
  // plan if if the frame is to be redrawn
  // or if we have completed the last subframe but not the entire frame
  if (m_bCompleteRedraw || 
     (m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame && m_iCurrentLODOffset > GetMinLODForCurrentView())) {

    /// \todo change this to the right subarea
    SetRenderTargetArea(RA_FULLSCREEN);

    // compute modelviewmatrix and forward to culling object
    FLOATMATRIX4 trans;
    trans.Translation(0,0,-2+m_fZoom);
    m_matModelView = m_Rot*trans;
    m_FrustumCulling.Update(m_matModelView);

    // compute current LOD level
    m_iCurrentLODOffset--;
    m_iCurrentLOD = std::min<UINT64>(m_iCurrentLODOffset,m_pDataset->GetInfo()->GetLODLevelCount()-1);
    UINT64VECTOR3 vBrickCount = m_pDataset->GetInfo()->GetBrickCount(m_iCurrentLOD);

    // build new todo brick list
    m_vCurrentBrickList = BuildFrameBrickList();
    m_iBricksRenderedInThisSubFrame = 0;
  }
}


void GPUSBVR::ExecuteFrame() {

  // clear the framebuffer if instructed
  if (m_bClearFramebuffer) {
    SetRenderTargetArea(RA_FULLSCREEN);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_FALSE);
    if (m_vBackgroundColors[0] == m_vBackgroundColors[1]) {
      glClearColor(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,0);
      glClear(GL_COLOR_BUFFER_BIT); 
    } else DrawBackGradient();
    //DrawLogo();
    glDepthMask(GL_TRUE);
  }

  if (m_bCompleteRedraw) {
    // update frame states
    m_iIntraFrameCounter = 0;
    m_iFrameCounter = m_pMasterController->MemMan()->UpdateFrameCounter();
    m_bCompleteRedraw = false;
  }

  // if there is something left in the TODO list
  if (m_vCurrentBrickList.size() > m_iBricksRenderedInThisSubFrame) {
    // setup shaders vars
    SetDataDepShaderVars(); 

    // bind offscreen buffer
    m_pFBO3DImageCurrent->Write();

    // clear target at the beginning
    if (m_iBricksRenderedInThisSubFrame == 0) {
      glClearColor(0,0,0,0);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }

    // render the images
    switch (m_eViewMode) {
      case VM_SINGLE    :  RenderSingle(); break;
      case VM_TWOBYTWO  :  Render2by2();   break;
      case VM_INVALID   :  m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid viewmode set"); break;
    }

    // unbind offscreen buffer
    m_pFBO3DImageCurrent->FinishWrite();

    // if there is nothing left todo in this subframe -> present the result
    if (m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame) swap(m_pFBO3DImageLast, m_pFBO3DImageCurrent);

  } else m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","Quick Redraw");


  // show the result
  RerenderPreviousResult();
}

void GPUSBVR::Paint() {
  PlanFrame();
  ExecuteFrame();
}

void GPUSBVR::Resize(const UINTVECTOR2& vWinSize) {
  AbstrRenderer::Resize(vWinSize);
  m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","Resizing to %i x %i", vWinSize.x, vWinSize.y);
  CreateOffscreenBuffer();
}

void GPUSBVR::SetRenderTargetArea(ERenderArea eREnderArea) {
  switch (eREnderArea) {
    case RA_TOPLEFT     : SetViewPort(UINTVECTOR2(0,m_vWinSize.y/2), m_vWinSize); break;
    case RA_TOPRIGHT    : SetViewPort(m_vWinSize/2, m_vWinSize); break;
    case RA_LOWERLEFT   : SetViewPort(UINTVECTOR2(0,0),m_vWinSize/2); break;
    case RA_LOWERRIGHT  : SetViewPort(UINTVECTOR2(m_vWinSize.x/2,0), m_vWinSize); break;
    case RA_FULLSCREEN  : SetViewPort(UINTVECTOR2(0,0), m_vWinSize); break;
    default             : m_pMasterController->DebugOut()->Error("GPUSBVR::SetRenderTargetArea","Invalid render area set"); break;
  }
}

void GPUSBVR::SetViewPort(UINTVECTOR2 viLowerLeft, UINTVECTOR2 viUpperRight) {

  UINTVECTOR2 viSize = viUpperRight-viLowerLeft;

  float aspect=(float)viSize.x/(float)viSize.y;
	glViewport(viLowerLeft.x,viLowerLeft.y,viSize.x,viSize.y);
	glMatrixMode(GL_PROJECTION);		
	glLoadIdentity();
	gluPerspective(50.0,aspect,0.1,100.0); 	// Set Projection. Arguments are FOV (in degrees), aspect-ratio, near-plane, far-plane.
  m_ArcBall.SetWindowSize(viSize.x, viSize.y);
  m_ArcBall.SetWindowOffset(viLowerLeft.x,viLowerLeft.y);
  
  // forward the GL projection matrix to the culling object
  FLOATMATRIX4 mProjection;
  mProjection.getProjection();
  m_FrustumCulling.SetParameters(mProjection);

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
}


void GPUSBVR::RerenderPreviousResult() {
	glViewport(0,0,m_vWinSize.x,m_vWinSize.y);

  m_pFBO3DImageLast->Read(GL_TEXTURE0);
  m_pFBO3DImageLast->ReadDepth(GL_TEXTURE1);

  m_pProgramTrans->Enable();

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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