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

using namespace std;

GPUSBVR::GPUSBVR(MasterController* pMasterController) :
  GLRenderer(pMasterController),
  m_pFBO3DImage(NULL)
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

  float fStepScale = m_SBVRGeogen.GetOpacityCorrection() * MathTools::Pow2(iCurrentLOD);

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

void GPUSBVR::Render1by3() {

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


vector<Brick> GPUSBVR::BuildFrameBrickList(UINT64 iCurrentLOD) {
  vector<Brick> vBrickList;

  UINT64VECTOR3 vOverlap = m_pDataset->GetInfo()->GetBrickOverlapSize();
  UINT64VECTOR3 vBrickDimension = m_pDataset->GetInfo()->GetBrickCount(iCurrentLOD);
  UINT64VECTOR3 vDomainSize = m_pDataset->GetInfo()->GetDomainSize(iCurrentLOD);
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
        
        UINT64VECTOR3 vSize = m_pDataset->GetInfo()->GetBrickSize(iCurrentLOD, UINT64VECTOR3(x,y,z));
        b = Brick(x,y,z, (unsigned int)(vSize.x), (unsigned int)(vSize.y), (unsigned int)(vSize.z));


        FLOATVECTOR3 vEffectiveSize = m_pDataset->GetInfo()->GetEffectiveBrickSize(iCurrentLOD, UINT64VECTOR3(x,y,z));


        b.vExtension.x = float(vEffectiveSize.x/float(iMaxDomainSize) * vScale.x);
        b.vExtension.y = float(vEffectiveSize.y/float(iMaxDomainSize) * vScale.y);
        b.vExtension.z = float(vEffectiveSize.z/float(iMaxDomainSize) * vScale.z);
        
        // compute center of the brick
        b.vCenter = (vBrickCorner + b.vExtension/2.0f)-vDomainExtend*0.5f;

        vBrickCorner.x += b.vExtension.x;


        // if the brick is visible (i.e. inside the frustum) add it to the list of active bricks
        if (m_FrustumCulling.IsVisible(b.vCenter, b.vExtension)) {

          b.vTexcoordsMin = FLOATVECTOR3((x == 0) ? 0.5f/b.vVoxelCount.x : vOverlap.x*0.5f/b.vVoxelCount.x,
                                         (y == 0) ? 0.5f/b.vVoxelCount.y : vOverlap.y*0.5f/b.vVoxelCount.y,
                                         (z == 0) ? 0.5f/b.vVoxelCount.z : vOverlap.z*0.5f/b.vVoxelCount.z);
          b.vTexcoordsMax = FLOATVECTOR3((x == vBrickDimension.x-1) ? 1.0f-0.5f/b.vVoxelCount.x : 1.0f-vOverlap.x*0.5f/b.vVoxelCount.x,
                                         (y == vBrickDimension.y-1) ? 1.0f-0.5f/b.vVoxelCount.y : 1.0f-vOverlap.y*0.5f/b.vVoxelCount.y,
                                         (z == vBrickDimension.z-1) ? 1.0f-0.5f/b.vVoxelCount.z : 1.0f-vOverlap.z*0.5f/b.vVoxelCount.z);
          
          vBrickList.push_back(b);
        }
      }

      vBrickCorner.x  = 0;
      vBrickCorner.y += b.vExtension.y;
    }
    vBrickCorner.y = 0;
    vBrickCorner.z += b.vExtension.z;
  }

  // Todo Sort list

  return vBrickList;
}

void GPUSBVR::Render3DView() {
  // compute current LOD level
  m_iCurrentLODOffset = (m_iCurrentLODOffset < 1) ? 0 : m_iCurrentLODOffset-1;
  UINT64 iCurrentLOD = std::min<UINT64>(m_iCurrentLODOffset,m_pDataset->GetInfo()->GetLODLevelCount()-1);
  UINT64VECTOR3 vBrickCount = m_pDataset->GetInfo()->GetBrickCount(iCurrentLOD);

  // ************** GL States ***********
  // Modelview
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  FLOATMATRIX4 trans;
  trans.Translation(0,0,-2+m_fZoom);
  m_matModelView = m_Rot*trans;
  m_matModelView.setModelview();

  // forward modelviewmatrix to culling object
  if (!m_bRenderGlobalBBox) m_FrustumCulling.Update(m_matModelView);

  // build brick list and sort by depth
  vector<Brick> vCurrentBrickList = BuildFrameBrickList(iCurrentLOD);

  // Blending & textures
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  // for rendering modes other than isosurface render the bbox once to init the depth buffer
  // for isosurface rendering we can go ahead and render the bbox directly as isosurfacing 
  // writes out correct depth values
  if (m_eRenderMode != RM_ISOSURFACE) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
  if (m_bRenderGlobalBBox) RenderBBox();
  if (m_bRenderLocalBBox) {
    for (UINT64 iCurrentBrick = 0;iCurrentBrick<vCurrentBrickList.size();iCurrentBrick++) {
      RenderBBox(FLOATVECTOR4(0,1,0,1), vCurrentBrickList[iCurrentBrick].vCenter, vCurrentBrickList[iCurrentBrick].vExtension);
    }
  }
  if (m_eRenderMode != RM_ISOSURFACE) glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

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
  for (UINT64 iCurrentBrick = 0;iCurrentBrick<vCurrentBrickList.size();iCurrentBrick++) {
    // m_pMasterController->DebugOut()->Message("GPUSBVR::Render3DView","Brick %i of %i", iCurrentBrick+1, vCurrentBrickList.size()); 
  
    // setup the slice generator
    m_SBVRGeogen.SetVolumeData(vCurrentBrickList[iCurrentBrick].vExtension, vCurrentBrickList[iCurrentBrick].vVoxelCount, 
                               vCurrentBrickList[iCurrentBrick].vTexcoordsMin, vCurrentBrickList[iCurrentBrick].vTexcoordsMax);
    FLOATMATRIX4 maBricktTrans; 
    maBricktTrans.Translation(vCurrentBrickList[iCurrentBrick].vCenter.x, vCurrentBrickList[iCurrentBrick].vCenter.y, vCurrentBrickList[iCurrentBrick].vCenter.z);
    FLOATMATRIX4 maBricktModelView = maBricktTrans * m_matModelView;
    maBricktModelView.setModelview();
    m_SBVRGeogen.SetTransformation(maBricktModelView);

    // convert 3D variables to the more general ND scheme used in the memory manager, e.i. convert 3-vectors to stl vectors
    vector<UINT64> vLOD; vLOD.push_back(iCurrentLOD);
    vector<UINT64> vBrick; 
    vBrick.push_back(vCurrentBrickList[iCurrentBrick].vCoords.x);
    vBrick.push_back(vCurrentBrickList[iCurrentBrick].vCoords.y);
    vBrick.push_back(vCurrentBrickList[iCurrentBrick].vCoords.z);

    // get the 3D texture from the memory manager
    GLTexture3D* t = m_pMasterController->MemMan()->Get3DTexture(m_pDataset, vLOD, vBrick, m_iIntraFrameCounter++, m_iFrameCounter);
    if(t!=NULL) t->Bind(0);

    // update the shader parameter
    SetBrickDepShaderVars(iCurrentLOD, vCurrentBrickList[iCurrentBrick]);

    // render the slices
    glBegin(GL_TRIANGLES);
      for (int i = int(m_SBVRGeogen.m_vSliceTriangles.size())-1;i>=0;i--) {
        glTexCoord3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vTex);
        glVertex3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vPos);
      }
    glEnd();

    // release the 3D texture
    m_pMasterController->MemMan()->Release3DTexture(t);

  }

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
    case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
    case RM_ISOSURFACE :  m_pProgramIso->Disable(); break;
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Render3DView","Invalid rendermode set"); break;
  }

  if (m_eRenderMode != RM_ISOSURFACE) {    
    m_matModelView.setModelview();
    if (m_bRenderGlobalBBox) {
      glDisable(GL_DEPTH_TEST);
      RenderBBox();
    }

    if (m_bRenderLocalBBox) {
      glDisable(GL_DEPTH_TEST);
      for (UINT64 iCurrentBrick = 0;iCurrentBrick<vCurrentBrickList.size();iCurrentBrick++) {
        RenderBBox(FLOATVECTOR4(0,1,0,1), vCurrentBrickList[iCurrentBrick].vCenter, vCurrentBrickList[iCurrentBrick].vExtension);
      }
    }
    glDepthMask(GL_TRUE);
  }

  glDisable(GL_BLEND);

}



void GPUSBVR::Paint(bool bClear) {

  SetDataDepShaderVars();

  if (bClear) {
    glClear(GL_DEPTH_BUFFER_BIT);
    if (m_vBackgroundColors[0] == m_vBackgroundColors[1]) {
      glClearColor(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,0);
      glClear(GL_COLOR_BUFFER_BIT); 
    } else DrawBackGradient();
    //DrawLogo();
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  if (m_bRedraw) {
    m_iIntraFrameCounter = 0;
    m_iFrameCounter = m_pMasterController->MemMan()->UpdateFrameCounter();

    m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","Complete Redraw");

    m_pFBO3DImage->Write();

    glClearColor(0,0,0,0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    switch (m_eViewMode) {
      case VM_SINGLE    :  RenderSingle(); break;
      case VM_TWOBYTWO  :  Render2by2();   break;
      case VM_ONEBYTREE :  Render1by3();   break;
      case VM_INVALID   :  m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid viewmode set"); break;
    }

    m_pFBO3DImage->FinishWrite();

  } else m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","Quick Redraw");

  RerenderPreviousResult();

  m_bRedraw = false;
}

void GPUSBVR::Resize(const UINTVECTOR2& vWinSize) {
  AbstrRenderer::Resize(vWinSize);

  m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","Resizing to %i x %i", vWinSize.x, vWinSize.y);

  float aspect=(float)vWinSize.x/(float)vWinSize.y;
	glViewport(0,0,vWinSize.x,vWinSize.y);
	glMatrixMode(GL_PROJECTION);		
	glLoadIdentity();
	gluPerspective(50.0,aspect,0.2,100.0); 	// Set Projection. Arguments are FOV (in degrees), aspect-ratio, near-plane, far-plane.
  
  // forward the GL projection matrix to the culling object
  FLOATMATRIX4 mProjection;
  mProjection.getProjection();
  m_FrustumCulling.SetParameters(mProjection);

	glMatrixMode(GL_MODELVIEW);

  if (m_pFBO3DImage != NULL) m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImage);
  m_pFBO3DImage = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, vWinSize.x, vWinSize.y, GL_RGBA8, 8*4, true);

}

void GPUSBVR::Cleanup() {
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[0]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[1]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[2]);

  m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImage);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram1DTrans[0]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram1DTrans[1]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram2DTrans[0]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgram2DTrans[1]);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramIso);
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramTrans);
}


void GPUSBVR::RerenderPreviousResult() {
  m_pFBO3DImage->Read(GL_TEXTURE0);
  m_pFBO3DImage->ReadDepth(GL_TEXTURE1);

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

  m_pFBO3DImage->FinishRead();
  m_pFBO3DImage->FinishDepthRead();
  glEnable(GL_DEPTH_TEST);
}


void GPUSBVR::SetSampleRateModifier(float fSampleRateModifier) {
  GLRenderer::SetSampleRateModifier(fSampleRateModifier);
  m_SBVRGeogen.SetSamplingModifier(fSampleRateModifier);
}