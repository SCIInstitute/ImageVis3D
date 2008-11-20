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
  GLRenderer(pMasterController)
{
}

GLRaycaster::~GLRaycaster() {
}


void GLRaycaster::Cleanup() {
  GLRenderer::Cleanup();

  m_pMasterController->MemMan()->FreeFBO(m_pFBOScratchpad);  
  m_pMasterController->MemMan()->FreeGLSLProgram(m_pRenderFrontFaces);
}

void GLRaycaster::CreateOffscreenBuffers() {
  GLRenderer::CreateOffscreenBuffers();

  if (m_pFBOScratchpad != NULL) m_pMasterController->MemMan()->FreeFBO(m_pFBOScratchpad);

  if (m_vWinSize.area() > 0) {
    m_pFBOScratchpad = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, m_vWinSize.x, m_vWinSize.y, GL_RGBA16F_ARB, 16*4, false);
  }
}


bool GLRaycaster::Initialize() {
  if (!GLRenderer::Initialize()) {
    m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error in parent call -> aborting");
    return false;
  }

  glShadeModel(GL_SMOOTH);
 
  if (!LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-frontfaces-FS.glsl",&(m_pRenderFrontFaces)) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-FS.glsl",        &(m_pProgram1DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-1D-light-FS.glsl",  &(m_pProgram1DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-FS.glsl",        &(m_pProgram2DTrans[0])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-2D-light-FS.glsl",  &(m_pProgram2DTrans[1])) ||
      !LoadAndVerifyShader("Shaders/GLRaycaster-VS.glsl", "Shaders/GLRaycaster-ISO-FS.glsl",       &m_pProgramIso)) {

      m_pMasterController->DebugOut()->Error("GLRaycaster::Initialize","Error loading a shader.");
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
  }

  return true;
}

void GLRaycaster::SetBrickDepShaderVars(const Brick& currentBrick) {

  FLOATVECTOR3 vStep(1.0f/currentBrick.vVoxelCount.x, 1.0f/currentBrick.vVoxelCount.y, 1.0f/currentBrick.vVoxelCount.z);

  float fStepScale = currentBrick.vVoxelCount.maxVal() * 0.5f * 1.0f/m_fSampleRateModifier * (currentBrick.vExtension/FLOATVECTOR3(currentBrick.vVoxelCount)).minVal();

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
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    // FRONT
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    // LEFT
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    // RIGHT
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    // BOTTOM
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMaxPoint.z);
    glTexCoord3f( vMinCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMinPoint.y, vMinPoint.z);
    glTexCoord3f( vMaxCoords.x, vMinCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMinPoint.y, vMinPoint.z);
    // TOP
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMaxCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMinPoint.z);
    glTexCoord3f( vMinCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMinPoint.x, vMaxPoint.y, vMaxPoint.z);
    glTexCoord3f( vMaxCoords.x, vMaxCoords.y, vMinCoords.z);
    glVertex3f( vMaxPoint.x, vMaxPoint.y, vMaxPoint.z);
  glEnd();
}



void GLRaycaster::Render3DPreLoop() {
  glEnable(GL_CULL_FACE);
}

void GLRaycaster::Render3DInLoop(size_t iCurrentBrick) {
  m_pRenderFrontFaces->Enable();
  RenderBox(m_vCurrentBrickList[iCurrentBrick].vCenter, m_vCurrentBrickList[iCurrentBrick].vExtension, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMin, m_vCurrentBrickList[iCurrentBrick].vTexcoordsMax, true);
  m_pRenderFrontFaces->Disable();
}


void GLRaycaster::Render3DPostLoop() {
  glDisable(GL_CULL_FACE);
}