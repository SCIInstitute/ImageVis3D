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
  \file    GLRaycaster.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    August 2008
*/


#include "GLRaycaster.h"

#include <cmath>
#include <Basics/SysTools.h>
#include <Controller/MasterController.h>
#include <ctime>

using namespace std;

GLRaycaster::GLRaycaster(MasterController* pMasterController) :
  GLRenderer(pMasterController),
  m_pFBOScratchpad(NULL),
  m_pFBOIsoHit(NULL),
  m_pRenderFrontFaces(NULL),
  m_pIsoCompose(NULL)
{
}

GLRaycaster::~GLRaycaster() {
}


void GLRaycaster::Cleanup() {
  GLRenderer::Cleanup();

  if (m_pFBOScratchpad)    {m_pMasterController->MemMan()->FreeFBO(m_pFBOScratchpad);  m_pFBOScratchpad = NULL;}
  if (m_pFBOIsoHit)        {m_pMasterController->MemMan()->FreeFBO(m_pFBOIsoHit);  m_pFBOIsoHit = NULL;}
  if (m_pRenderFrontFaces) {m_pMasterController->MemMan()->FreeGLSLProgram(m_pRenderFrontFaces); m_pRenderFrontFaces = NULL;}
  if (m_pIsoCompose)       {m_pMasterController->MemMan()->FreeGLSLProgram(m_pIsoCompose); m_pIsoCompose = NULL;}
}

void GLRaycaster::CreateOffscreenBuffers() {
  GLRenderer::CreateOffscreenBuffers();

  if (m_pFBOScratchpad) {m_pMasterController->MemMan()->FreeFBO(m_pFBOScratchpad); m_pFBOIsoHit = NULL;}
  if (m_pFBOIsoHit)     {m_pMasterController->MemMan()->FreeFBO(m_pFBOIsoHit);  m_pFBOIsoHit = NULL;}

  if (m_vWinSize.area() > 0) {
    m_pFBOScratchpad = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, false);
    m_pFBOIsoHit     = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, false, 2);
  }
}


bool GLRaycaster::Initialize() {
  if (!GLRenderer::Initialize()) {
    m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error in parent call -> aborting");
    return false;
  }

  glShadeModel(GL_SMOOTH);
 
  if (!LoadAndVerifyShader("Shaders/Transfer-VS.glsl",    "Shaders/GLRaycaster-compose-FS.glsl",   &(m_pIsoCompose)) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-frontfaces-FS.glsl",&(m_pRenderFrontFaces)) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-FS.glsl",        &(m_pProgram1DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-light-FS.glsl",  &(m_pProgram1DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-FS.glsl",        &(m_pProgram2DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-light-FS.glsl",  &(m_pProgram2DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-ISO-FS.glsl",       &(m_pProgramIso))) {

      Cleanup();

      m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error loading a shader.");
      return false;
  } else {
    m_pProgram1DTrans[0]->Enable();
    m_pProgram1DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[0]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[0]->SetUniformVector("texRayEntry",2);
    m_pProgram1DTrans[0]->Disable();

    m_pProgram1DTrans[1]->Enable();
    m_pProgram1DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[1]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[1]->SetUniformVector("texRayEntry",2);
    m_pProgram1DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram1DTrans[1]->Disable();

    m_pProgram2DTrans[0]->Enable();
    m_pProgram2DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[0]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[0]->SetUniformVector("texRayEntry",2);
    m_pProgram2DTrans[0]->Disable();

    m_pProgram2DTrans[1]->Enable();
    m_pProgram2DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[1]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[1]->SetUniformVector("texRayEntry",2);
    m_pProgram2DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram2DTrans[1]->Disable();

    m_pProgramIso->Enable();
    m_pProgramIso->SetUniformVector("texVolume",0);
    m_pProgramIso->SetUniformVector("texRayEntry",2);
    m_pProgramIso->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgramIso->SetUniformVector("vLightDiffuse",0.8f,0.8f,0.8f);
    m_pProgramIso->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgramIso->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);

    FLOATVECTOR2 vParams = m_FrustumCullingLOD.GetDepthScaleParams();
    m_pProgramIso->SetUniformVector("vProjParam",vParams.x, vParams.y);
    m_pProgramIso->Disable();
  }

  return true;
}

void GLRaycaster::SetBrickDepShaderVars(const Brick& currentBrick) {
  FLOATVECTOR3 vStep(1.0f/currentBrick.vVoxelCount.x, 1.0f/currentBrick.vVoxelCount.y, 1.0f/currentBrick.vVoxelCount.z);
  float fRayStep = (vStep * 0.5f * 1.0f/m_fSampleRateModifier).length();
  float fStepScale = 1.0f/m_fSampleRateModifier * (FLOATVECTOR3(m_pDataset->GetInfo()->GetDomainSize())/FLOATVECTOR3(m_pDataset->GetInfo()->GetDomainSize(m_iCurrentLOD))).maxVal();;

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {                    
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fRayStepsize", fRayStep);
                            if (m_bUseLigthing)
                                m_pProgram1DTrans[1]->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            break;
                          }
    case RM_2DTRANS    :  {
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fRayStepsize", fRayStep);
                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
                            m_pProgramIso->SetUniformVector("fRayStepsize", fRayStep);
                            break;
                          }
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GLRaycaster::SetBrickDepShaderVars","Invalid rendermode set"); break;
  }

}

const FLOATVECTOR2 GLRaycaster::SetDataDepShaderVars() {
  const FLOATVECTOR2 vSizes = GLRenderer::SetDataDepShaderVars();

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fTransScale",vSizes.x);
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable();
                            break;
                          }
    case RM_2DTRANS    :  {
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fTransScale",vSizes.x);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fGradientScale",vSizes.y);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable();
                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->Enable();
                            m_pProgramIso->SetUniformVector("fIsoval",m_fIsovalue/vSizes.x);
                            m_pProgramIso->Disable();
                            break;
                          }
    case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GLRaycaster::SetDataDepShaderVars","Invalid rendermode set"); break;
  }


  return vSizes;
}

void GLRaycaster::RenderBox(const FLOATVECTOR3& vCenter, const FLOATVECTOR3& vExtend, const FLOATVECTOR3& vMinCoords, const FLOATVECTOR3& vMaxCoords, bool bCullBack) {
  if (bCullBack) {
    glCullFace(GL_BACK);
  } else {
    glCullFace(GL_FRONT);
  }

  FLOATVECTOR3 vMinPoint, vMaxPoint;
  vMinPoint = (vCenter - vExtend/2.0);
  vMaxPoint = (vCenter + vExtend/2.0);

  glBegin(GL_QUADS);        
    // BACK
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    // FRONT
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    // LEFT
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    // RIGHT
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    // BOTTOM
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    // TOP
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
  glEnd();
}

void GLRaycaster::Render3DPreLoop() {
  glEnable(GL_CULL_FACE);

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_p1DTransTex->Bind(1); 
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_2DTRANS    :  m_p2DTransTex->Bind(1);
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_ISOSURFACE :  break;
    default    :          m_pMasterController->DebugOut()->Error("GLSBVR::Render3DView","Invalid rendermode set"); 
                          break;
  }

  if (m_eRenderMode != RM_ISOSURFACE) glDepthMask(GL_FALSE);
}

void GLRaycaster::Render3DInLoop(size_t iCurrentBrick) {

  // disable writing to the main offscreen buffer
  m_pFBO3DImageCurrent->FinishWrite();

  // write frontfaces (ray entry points) into scratchpad
  m_pFBOScratchpad->Write();
  m_pRenderFrontFaces->Enable();
  RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, true);
  m_pRenderFrontFaces->Disable();
  m_pFBOScratchpad->FinishWrite();


  if (m_eRenderMode == RM_ISOSURFACE) {
    m_pProgramIso->Enable(); 
    SetBrickDepShaderVars(m_vCurrentBrickList[iCurrentBrick]);
    
    m_pFBOIsoHit->Write(GL_COLOR_ATTACHMENT0_EXT, 0);
    m_pFBOIsoHit->Write(GL_COLOR_ATTACHMENT1_EXT, 1);

    // compute the hit position and normal
    m_pFBOScratchpad->Read(GL_TEXTURE2);
    RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, false);
    m_pFBOScratchpad->FinishRead();

    m_pProgramIso->Disable(); 

    m_pFBOIsoHit->FinishWrite(0);
    m_pFBOIsoHit->FinishWrite(1);

    m_pFBO3DImageCurrent->Write();
  } else {
    m_pFBO3DImageCurrent->Write();

    // do the raycasting
    switch (m_eRenderMode) {
      case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Enable();
                            glEnable(GL_BLEND);
                            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                            break;
      case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Enable(); 
                            glEnable(GL_BLEND);
                            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                            break;
      default            :  m_pMasterController->DebugOut()->Error("GLRaycaster::Render3DInLoop","Invalid rendermode set"); 
                            break;
    }

    SetBrickDepShaderVars(m_vCurrentBrickList[iCurrentBrick]);

    m_pFBOScratchpad->Read(GL_TEXTURE2);
    RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, false);
    m_pFBOScratchpad->FinishRead();
    switch (m_eRenderMode) {
      case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable();
                            glDisable(GL_BLEND);
                            break;
      case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable(); 
                            glDisable(GL_BLEND);
                            break;
      default            :  m_pMasterController->DebugOut()->Error("GLRaycaster::Render3DInLoop","Invalid rendermode set"); 
                            break;
    }
  }
}


void GLRaycaster::Render3DPostLoop() {
  glDisable(GL_CULL_FACE);
  glDepthMask(GL_TRUE);
}

void GLRaycaster::Resize(const UINTVECTOR2& vWinSize) {
  GLRenderer::Resize(vWinSize);

  FLOATVECTOR2 vfWinSize = FLOATVECTOR2(vWinSize);
  m_pProgram1DTrans[0]->Enable();
  m_pProgram1DTrans[0]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
  m_pProgram1DTrans[0]->Disable();

  m_pProgram1DTrans[1]->Enable();
  m_pProgram1DTrans[1]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
  m_pProgram1DTrans[1]->Disable();

  m_pProgram2DTrans[0]->Enable();
  m_pProgram2DTrans[0]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
  m_pProgram2DTrans[0]->Disable();

  m_pProgram2DTrans[1]->Enable();
  m_pProgram2DTrans[1]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
  m_pProgram2DTrans[1]->Disable();

  m_pProgramIso->Enable();
  m_pProgramIso->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
  m_pProgramIso->Disable();
}
