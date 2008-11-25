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
  m_pFBORayEntry(NULL),
  m_pFBOIsoHit(NULL),
  m_pFBOCVHit(NULL),
  m_pProgramRenderFrontFaces(NULL),
  m_pProgramIsoCompose(NULL),
  m_pProgramCV(NULL),
  m_pProgramCVCompose(NULL)
{
}

GLRaycaster::~GLRaycaster() {
}


void GLRaycaster::Cleanup() {
  GLRenderer::Cleanup();

  if (m_pFBORayEntry){
    m_pMasterController->MemMan()->FreeFBO(m_pFBORayEntry); 
    m_pFBORayEntry = NULL;
  }
  if (m_pFBOIsoHit){
    m_pMasterController->MemMan()->FreeFBO(m_pFBOIsoHit);                      
    m_pFBOIsoHit = NULL;
  }
  if (m_pFBOCVHit){
    m_pMasterController->MemMan()->FreeFBO(m_pFBOCVHit);                      
    m_pFBOCVHit = NULL;
  }
  if (m_pProgramRenderFrontFaces){
    m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramRenderFrontFaces); 
    m_pProgramRenderFrontFaces = NULL;
  }
  if (m_pProgramIsoCompose){
    m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramIsoCompose); 
    m_pProgramIsoCompose = NULL;
  }
  if (m_pProgramCV){
    m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramCV); 
    m_pProgramCV = NULL;
  }
  if (m_pProgramCVCompose){
    m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramCVCompose); 
    m_pProgramCVCompose = NULL;
  }
}

void GLRaycaster::CreateOffscreenBuffers() {
  GLRenderer::CreateOffscreenBuffers();

  if (m_pFBORayEntry) {
    m_pMasterController->MemMan()->FreeFBO(m_pFBORayEntry); 
    m_pFBORayEntry = NULL;
  }
  if (m_pFBOIsoHit) {
    m_pMasterController->MemMan()->FreeFBO(m_pFBOIsoHit);
    m_pFBOIsoHit = NULL;
  }
  if (m_pFBOCVHit){
    m_pMasterController->MemMan()->FreeFBO(m_pFBOCVHit);                      
    m_pFBOCVHit = NULL;
  }
  if (m_vWinSize.area() > 0) {
    m_pFBORayEntry = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, false, 2);
    m_pFBOIsoHit   = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, true, 2);
    m_pFBOCVHit    = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, true, 2);
  }
}


bool GLRaycaster::Initialize() {
  if (!GLRenderer::Initialize()) {
    m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error in parent call -> aborting");
    return false;
  }

  glShadeModel(GL_SMOOTH);
 
  if (!LoadAndVerifyShader("Shaders/Transfer-VS.glsl",    "Shaders/GLRaycaster-compose-FS.glsl",   &(m_pProgramIsoCompose)) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-frontfaces-FS.glsl",&(m_pProgramRenderFrontFaces)) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-FS.glsl",        &(m_pProgram1DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-light-FS.glsl",  &(m_pProgram1DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-FS.glsl",        &(m_pProgram2DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-light-FS.glsl",  &(m_pProgram2DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-ISO-FS.glsl",       &(m_pProgramIso)) || 
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-ISO-CV-FS.glsl",    &(m_pProgramCV))  ||
      !LoadAndVerifyShader("Shaders/Transfer-VS.glsl",    "Shaders/GLRaycaster-compose-CV-FS.glsl",&(m_pProgramCVCompose))) {

      Cleanup();

      m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error loading a shader.");
      return false;
  } else {

    m_pProgram1DTrans[0]->Enable();
    m_pProgram1DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[0]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[0]->SetUniformVector("texRayExit",2);
    m_pProgram1DTrans[0]->SetUniformVector("texRayExitPos",3);
    m_pProgram1DTrans[0]->Disable();

    m_pProgram1DTrans[1]->Enable();
    m_pProgram1DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[1]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[1]->SetUniformVector("texRayExit",2);
    m_pProgram1DTrans[1]->SetUniformVector("texRayExitPos",3);
    m_pProgram1DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram1DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram1DTrans[1]->Disable();

    m_pProgram2DTrans[0]->Enable();
    m_pProgram2DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[0]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[0]->SetUniformVector("texRayExit",2);
    m_pProgram2DTrans[0]->SetUniformVector("texRayExitPos",3);
    m_pProgram2DTrans[0]->Disable();

    m_pProgram2DTrans[1]->Enable();
    m_pProgram2DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[1]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[1]->SetUniformVector("texRayExit",2);
    m_pProgram2DTrans[1]->SetUniformVector("texRayExitPos",3);
    m_pProgram2DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgram2DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgram2DTrans[1]->Disable();

    m_pProgramIso->Enable();
    m_pProgramIso->SetUniformVector("texVolume",0);
    m_pProgramIso->SetUniformVector("texRayExit",2);
    m_pProgramIso->SetUniformVector("texRayExitPos",3);
    FLOATVECTOR2 vParams = m_FrustumCullingLOD.GetDepthScaleParams();
    m_pProgramIso->SetUniformVector("vProjParam",vParams.x, vParams.y);
    m_pProgramIso->Disable();

    m_pProgramIsoCompose->Enable();
    m_pProgramIsoCompose->SetUniformVector("texRayHitPos",0);
    m_pProgramIsoCompose->SetUniformVector("texRayHitNormal",1);
    m_pProgramIsoCompose->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgramIsoCompose->SetUniformVector("vLightDiffuse",0.8f,0.8f,0.8f);
    m_pProgramIsoCompose->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgramIsoCompose->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgramIsoCompose->SetUniformVector("vProjParam",vParams.x, vParams.y);
    m_pProgramIsoCompose->Disable();

    m_pProgramCVCompose->Enable();
    m_pProgramCVCompose->SetUniformVector("texRayHitPos",0);
    m_pProgramCVCompose->SetUniformVector("texRayHitNormal",1);
    m_pProgramCVCompose->SetUniformVector("texRayHitPos2",2);
    m_pProgramCVCompose->SetUniformVector("texRayHitNormal2",3);
    m_pProgramCVCompose->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
    m_pProgramCVCompose->SetUniformVector("vLightDiffuse",0.8f,0.8f,0.8f);
    m_pProgramCVCompose->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
    m_pProgramCVCompose->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
    m_pProgramCVCompose->SetUniformVector("vProjParam",vParams.x, vParams.y);
    m_pProgramCVCompose->Disable();

    m_pProgramCV->Enable();
    m_pProgramCV->SetUniformVector("texVolume",0);
    m_pProgramCV->SetUniformVector("texRayExit",2);
    m_pProgramCV->SetUniformVector("texRayExitPos",3);
    m_pProgramCV->SetUniformVector("texLastHit",4);
    m_pProgramCV->SetUniformVector("texLastHitPos",5);
    m_pProgramCV->Disable();    
  }

  return true;
}

void GLRaycaster::SetBrickDepShaderVars(size_t iCurrentBrick) {
  const Brick& currentBrick = m_vCurrentBrickList[iCurrentBrick];

  FLOATVECTOR3 vVoxelSizeTexSpace = 1.0f/FLOATVECTOR3(currentBrick.vVoxelCount);
  float fRayStep = (currentBrick.vExtension*vVoxelSizeTexSpace * 0.5f * 1.0f/m_fSampleRateModifier).minVal();
  float fStepScale = 1.0f/m_fSampleRateModifier * (FLOATVECTOR3(m_pDataset->GetInfo()->GetDomainSize())/FLOATVECTOR3(m_pDataset->GetInfo()->GetDomainSize(m_iCurrentLOD))).maxVal();

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fRayStepsize", fRayStep);
                            if (m_bUseLigthing)
                                m_pProgram1DTrans[1]->SetUniformVector("vVoxelStepsize", vVoxelSizeTexSpace.x, vVoxelSizeTexSpace.y, vVoxelSizeTexSpace.z);
                            break;
                          }
    case RM_2DTRANS    :  {
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("vVoxelStepsize", vVoxelSizeTexSpace.x, vVoxelSizeTexSpace.y, vVoxelSizeTexSpace.z);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fRayStepsize", fRayStep);
                            break;
                          }
    case RM_ISOSURFACE : {
                            if (m_bDoClearView) {
                              m_pProgramCV->Enable();
                              m_pProgramCV->SetUniformVector("vVoxelStepsize", vVoxelSizeTexSpace.x, vVoxelSizeTexSpace.y, vVoxelSizeTexSpace.z);
                              m_pProgramCV->SetUniformVector("fRayStepsize", fRayStep);
                              m_pProgramCV->SetUniformVector("iTileID", int(iCurrentBrick));
                              m_pProgramCV->Disable();
                              m_pProgramIso->Enable();
                            } 
                            m_pProgramIso->SetUniformVector("vVoxelStepsize", vVoxelSizeTexSpace.x, vVoxelSizeTexSpace.y, vVoxelSizeTexSpace.z);
                            m_pProgramIso->SetUniformVector("fRayStepsize", fRayStep);
                            m_pProgramIso->SetUniformVector("iTileID", int(iCurrentBrick));
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
                            if (m_bDoClearView) {
                              m_pProgramCV->Enable();
                              m_pProgramCV->SetUniformVector("fIsoval",m_fCVIsovalue/vSizes.x);
                              m_pProgramCV->Disable();
                            } 
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
    case RM_ISOSURFACE :  if (m_bDoClearView) {
                            m_pProgramCVCompose->Enable(); 
                            m_pProgramCVCompose->SetUniformVector("vLightDiffuse",m_vIsoColor.x, m_vIsoColor.y, m_vIsoColor.z);
                            m_pProgramCVCompose->SetUniformVector("vLightDiffuse2",m_vCVColor.x, m_vCVColor.y, m_vCVColor.z);
                            m_pProgramCVCompose->SetUniformVector("vCVParam",m_fCVSize, m_fCVContextScale, m_fCVBorderScale);
                            m_pProgramCVCompose->SetUniformVector("vCVPickPos", m_vCVPos.x, m_vCVPos.y);
                            m_pProgramCVCompose->Disable(); 
                            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          } else {
                            m_pProgramIsoCompose->Enable(); 
                            m_pProgramIsoCompose->SetUniformVector("vLightDiffuse",m_vIsoColor.x, m_vIsoColor.y, m_vIsoColor.z);
                            m_pProgramIsoCompose->Disable(); 
                          } 

                          break;
    default    :          m_pMasterController->DebugOut()->Error("GLSBVR::Render3DView","Invalid rendermode set"); 
                          break;
  }
}

void GLRaycaster::Render3DInLoop(size_t iCurrentBrick) {
  glDisable(GL_BLEND);
  glDepthMask(GL_FALSE);

  // disable writing to the main offscreen buffer
  m_pFBO3DImageCurrent->FinishWrite();

  // write frontfaces (ray entry points)
  m_pFBORayEntry->Write(GL_COLOR_ATTACHMENT0_EXT, 0);
  m_pFBORayEntry->Write(GL_COLOR_ATTACHMENT1_EXT, 1);

  GLenum twobuffers[]  = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
  glDrawBuffers(2, twobuffers);

  m_pProgramRenderFrontFaces->Enable();
  RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, false);
  m_pProgramRenderFrontFaces->Disable();

  glDrawBuffer(GL_NONE);

  GLenum e = glGetError();
  if (GL_NO_ERROR!= e) {
    m_pMasterController->DebugOut()->Error("GLFBOTex:FinishWrite","Error unbinding FBO 1!");
	}
  m_pFBORayEntry->FinishWrite(1);
  m_pFBORayEntry->FinishWrite(0);
 
  if (m_eRenderMode == RM_ISOSURFACE) { 
    glDepthMask(GL_TRUE);
    m_pFBOIsoHit->Write(GL_COLOR_ATTACHMENT0_EXT, 0);
    m_pFBOIsoHit->Write(GL_COLOR_ATTACHMENT1_EXT, 1);
    glDrawBuffers(2, twobuffers);

    if (m_iBricksRenderedInThisSubFrame == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_pProgramIso->Enable();
    SetBrickDepShaderVars(iCurrentBrick);
    m_pFBORayEntry->Read(GL_TEXTURE2_ARB, 0);
    m_pFBORayEntry->Read(GL_TEXTURE3_ARB, 1);
    RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, true);
    m_pFBORayEntry->FinishRead(1);
    m_pFBORayEntry->FinishRead(0);
    m_pProgramIso->Disable();

    glDrawBuffer(GL_NONE);

    m_pFBOIsoHit->FinishWrite(1);
    m_pFBOIsoHit->FinishWrite(0);

    if (m_bDoClearView) {
      m_pFBOCVHit->Write(GL_COLOR_ATTACHMENT0_EXT, 0);
      m_pFBOCVHit->Write(GL_COLOR_ATTACHMENT1_EXT, 1);
      glDrawBuffers(2, twobuffers);

      if (m_iBricksRenderedInThisSubFrame == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_pProgramCV->Enable();
      m_pFBORayEntry->Read(GL_TEXTURE2_ARB, 0);
      m_pFBORayEntry->Read(GL_TEXTURE3_ARB, 1);
      m_pFBOIsoHit->Read(GL_TEXTURE4_ARB, 0);
      m_pFBOIsoHit->Read(GL_TEXTURE5_ARB, 1);
      RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, true);
      m_pFBOIsoHit->FinishRead(1);
      m_pFBOIsoHit->FinishRead(0);
      m_pFBORayEntry->FinishRead(1);
      m_pFBORayEntry->FinishRead(0);
      m_pProgramCV->Disable();
      glDrawBuffer(GL_NONE);

      m_pFBOCVHit->FinishWrite(1);
      m_pFBOCVHit->FinishWrite(0);
    }

    m_pFBO3DImageCurrent->Write();
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  } else {
    m_pFBO3DImageCurrent->Write();
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

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

    SetBrickDepShaderVars(iCurrentBrick);

    m_pFBORayEntry->Read(GL_TEXTURE2_ARB, 0);
    m_pFBORayEntry->Read(GL_TEXTURE3_ARB, 1);
    RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, true);
    m_pFBORayEntry->FinishRead(0);
    m_pFBORayEntry->FinishRead(1);

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
  glEnable(GL_BLEND);

  if (m_eRenderMode == RM_ISOSURFACE && m_vCurrentBrickList.size() == m_iBricksRenderedInThisSubFrame) {    
    m_pFBOIsoHit->Read(GL_TEXTURE0, 0);
    m_pFBOIsoHit->Read(GL_TEXTURE1, 1);

    if (m_bDoClearView) {
      m_pFBOCVHit->Read(GL_TEXTURE2, 0);
      m_pFBOCVHit->Read(GL_TEXTURE3, 1);
      m_pProgramCVCompose->Enable();
    } else m_pProgramIsoCompose->Enable();

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

    if (m_bDoClearView) {
      m_pFBOCVHit->FinishRead(0);
      m_pFBOCVHit->FinishRead(1);
      m_pProgramCVCompose->Disable();
    } else m_pProgramIsoCompose->Disable();

    m_pFBOIsoHit->FinishRead(1);
    m_pFBOIsoHit->FinishRead(0);

    m_bPerformReCompose = false;
  }
}

void GLRaycaster::StartFrame() {
  FLOATVECTOR2 vfWinSize = FLOATVECTOR2(m_vWinSize);

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_pProgram1DTrans[0]->Enable();
                          m_pProgram1DTrans[0]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                          m_pProgram1DTrans[0]->Disable();

                          m_pProgram1DTrans[1]->Enable();
                          m_pProgram1DTrans[1]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                          m_pProgram1DTrans[1]->Disable();
                          break;
    case RM_2DTRANS    :  m_pProgram2DTrans[0]->Enable();
                          m_pProgram2DTrans[0]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                          m_pProgram2DTrans[0]->Disable();

                          m_pProgram2DTrans[1]->Enable();
                          m_pProgram2DTrans[1]->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                          m_pProgram2DTrans[1]->Disable();
                          break;
    case RM_ISOSURFACE :  if (m_bDoClearView) {
                            m_pProgramCVCompose->Enable();
                            m_pProgramCVCompose->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                            m_pProgramCVCompose->Disable();

                            m_pProgramCV->Enable();
                            m_pProgramCV->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                            m_pProgramCV->Disable();
                          } else {
                            m_pProgramIsoCompose->Enable();
                            m_pProgramIsoCompose->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                            m_pProgramIsoCompose->Disable();
                          } 
                          m_pProgramIso->Enable();
                          m_pProgramIso->SetUniformVector("vScreensize",vfWinSize.x, vfWinSize.y);
                          m_pProgramIso->Disable();
                          break;
    default    :          m_pMasterController->DebugOut()->Error("GLSBVR::StartFrame","Invalid rendermode set"); 
                          break;
  }

  GLRenderer::StartFrame();
}