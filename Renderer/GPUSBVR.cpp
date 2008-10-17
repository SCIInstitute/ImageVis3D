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
  m_bDelayedCompleteRedraw(false),
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


void GPUSBVR::Initialize() {
  GLRenderer::Initialize();

  m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","");

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

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","1");

  m_pProgram1DTrans[0]->Enable();
  m_pProgram1DTrans[0]->SetUniformVector("texVolume",0);
  m_pProgram1DTrans[0]->SetUniformVector("texTrans1D",1);
  m_pProgram1DTrans[0]->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","2");

  m_pProgram1DTrans[1]->Enable();
  m_pProgram1DTrans[1]->SetUniformVector("texVolume",0);
  m_pProgram1DTrans[1]->SetUniformVector("texTrans1D",1);
  m_pProgram1DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
  m_pProgram1DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
  m_pProgram1DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
  m_pProgram1DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
  m_pProgram1DTrans[1]->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","3");

  m_pProgram2DTrans[0]->Enable();
  m_pProgram2DTrans[0]->SetUniformVector("texVolume",0);
  m_pProgram2DTrans[0]->SetUniformVector("texTrans2D",1);
  m_pProgram2DTrans[0]->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","4");

  m_pProgram2DTrans[1]->Enable();
  m_pProgram2DTrans[1]->SetUniformVector("texVolume",0);
  m_pProgram2DTrans[1]->SetUniformVector("texTrans2D",1);
  m_pProgram2DTrans[1]->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
  m_pProgram2DTrans[1]->SetUniformVector("vLightDiffuse",1.0f,1.0f,1.0f);
  m_pProgram2DTrans[1]->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
  m_pProgram2DTrans[1]->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
  m_pProgram2DTrans[1]->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","5");

  m_pProgramIso->Enable();
  m_pProgramIso->SetUniformVector("texVolume",0);
  m_pProgramIso->SetUniformVector("vLightAmbient",0.2f,0.2f,0.2f);
  m_pProgramIso->SetUniformVector("vLightDiffuse",0.8f,0.8f,0.8f);
  m_pProgramIso->SetUniformVector("vLightSpecular",1.0f,1.0f,1.0f);
  m_pProgramIso->SetUniformVector("vLightDir",0.0f,0.0f,-1.0f);
  m_pProgramIso->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","6");

  m_pProgramTrans->Enable();
  m_pProgramTrans->SetUniformVector("texColor",0);
  m_pProgramTrans->SetUniformVector("texDepth",1);
  m_pProgramTrans->Disable();

  m_pMasterController->DebugOut()->Error("GPUSBVR::Initialize","7");

}

void GPUSBVR::SetBrickDepShaderVars(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) {

  UINTVECTOR3  vSize = m_pDataset->GetBrickSize(vLOD, vBrick);
  float fStepX = 1.0f/vSize.x;
  float fStepY = 1.0f/vSize.y;
  float fStepZ = 1.0f/vSize.z;

  float fStepScale = m_SBVRGeogen.GetOpacityCorrection();

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  {
                            m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            if (m_bUseLigthing)
                                m_pProgram1DTrans[1]->SetUniformVector("vVoxelStepsize", fStepX, fStepY, fStepZ);
                            break;
                          }
    case RM_2DTRANS    :  {
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("fStepScale", fStepScale);
                            m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->SetUniformVector("vVoxelStepsize", fStepX, fStepY, fStepZ);
                            break;
                          }
    case RM_ISOSURFACE : {
                            m_pProgramIso->SetUniformVector("vVoxelStepsize", fStepX, fStepY, fStepZ);
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


void GPUSBVR::UpdateGeoGen(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) {
  UINTVECTOR3  vSize = m_pDataset->GetBrickSize(vLOD, vBrick);
  FLOATVECTOR3 vCenter, vExtension;
  m_pDataset->GetBrickCenterAndExtension(vLOD, vBrick, vCenter, vExtension);

  /// \todo transfer brick offsets to m_SBVRGeogen
  m_SBVRGeogen.SetVolumeData(vExtension, vSize);
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

void GPUSBVR::RenderBBox() {
  glBegin(GL_LINES);
    glColor4f(1,0,0,1);
    // FRONT
    glVertex3f( 0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f( 0.5f,-0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f,-0.5f);

    // BACK
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);

    // CONNECTION
    glVertex3f(-0.5f,-0.5f, 0.5f);
    glVertex3f(-0.5f,-0.5f,-0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f,-0.5f);
    glVertex3f( 0.5f,-0.5f, 0.5f);
    glVertex3f( 0.5f,-0.5f,-0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f,-0.5f);
  glEnd();
}

void GPUSBVR::Render3DView() {

    // DEBUG: just render the smallest brick for a start
    std::vector<UINT64> vSmallestLOD = m_pDataset->GetInfo()->GetLODLevelCount();
    for (size_t i = 0;i<vSmallestLOD.size();i++) vSmallestLOD[i] -= 1;   
    std::vector<UINT64> vFirstBrick(m_pDataset->GetInfo()->GetBrickCount(vSmallestLOD).size());
    for (size_t i = 0;i<vFirstBrick.size();i++) vFirstBrick[i] = 0;
    UpdateGeoGen(vSmallestLOD, vFirstBrick);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    FLOATMATRIX4 trans;
    trans.Translation(0,0,-2+m_fZoom);
    m_matModelView = trans*m_Rot;

    m_matModelView.setModelview();
    m_SBVRGeogen.SetTransformation(m_matModelView);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_2D);
    if (m_eRenderMode != RM_ISOSURFACE) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    if (m_bRenderGlobalBBox) RenderBBox();
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
	                          glDisable(GL_BLEND);
                            break;
      case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid rendermode set"); 
                            break;
    }

    GLTexture3D* t = m_pMasterController->MemMan()->Get3DTexture(m_pDataset, vSmallestLOD, vFirstBrick);
    if(t!=NULL) t->Bind(0);

    SetBrickDepShaderVars(vSmallestLOD, vFirstBrick);

    if (m_eRenderMode != RM_ISOSURFACE) glDepthMask(GL_FALSE);

    glBegin(GL_TRIANGLES);
      for (int i = m_SBVRGeogen.m_vSliceTriangles.size()-1;i>=0;i--) {
        glTexCoord3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vTex);
        glVertex3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vPos);
      }
    glEnd();

    m_pMasterController->MemMan()->Release3DTexture(t);

    switch (m_eRenderMode) {
      case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
      case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLigthing ? 1 : 0]->Disable(); break;
      case RM_ISOSURFACE :  m_pProgramIso->Disable(); break;
      case RM_INVALID    :  m_pMasterController->DebugOut()->Error("GPUSBVR::Paint","Invalid rendermode set"); break;
    }

    if (m_eRenderMode != RM_ISOSURFACE) {    
      if (m_bRenderGlobalBBox) {
        glDisable(GL_DEPTH_TEST);
        RenderBBox();
      }
      glDepthMask(GL_TRUE);
    }

    glDisable(GL_BLEND);
}



void GPUSBVR::Paint(bool bClear) {

  if (bClear) {
    glClear(GL_DEPTH_BUFFER_BIT);
    SetDataDepShaderVars();
    if (m_vBackgroundColors[0] == m_vBackgroundColors[1]) {
      glClearColor(m_vBackgroundColors[0].x,m_vBackgroundColors[0].y,m_vBackgroundColors[0].z,0);
      glClear(GL_COLOR_BUFFER_BIT); 
    } else DrawBackGradient();
    DrawLogo();
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  if (m_bCompleteRedraw) {
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

  m_bCompleteRedraw = false;
  m_bRedraw = false;
}

void GPUSBVR::Resize(const UINTVECTOR2& vWinSize) {
  AbstrRenderer::Resize(vWinSize);

  m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","Resizing to %i x %i", vWinSize.x, vWinSize.y);

//  int side = std::min(width, height);
//  glViewport((width - side) / 2, (height - side) / 2, side, side);

  float aspect=(float)vWinSize.x/(float)vWinSize.y;
	glViewport(0,0,vWinSize.x,vWinSize.y);
	glMatrixMode(GL_PROJECTION);		
	glLoadIdentity();
	gluPerspective(50.0,aspect,0.2,100.0); 	// Set Projection. Arguments are FOV (in degrees), aspect-ratio, near-plane, far-plane.
	glMatrixMode(GL_MODELVIEW);

  if (m_pFBO3DImage != NULL) m_pMasterController->MemMan()->FreeFBO(m_pFBO3DImage);
  m_pFBO3DImage = m_pMasterController->MemMan()->GetFBO(GL_NEAREST, GL_NEAREST, GL_CLAMP, vWinSize.x, vWinSize.y, GL_RGBA8, 8*4, true);

  m_bDelayedCompleteRedraw = true;
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


bool GPUSBVR::CheckForRedraw() {
  if (m_bDelayedCompleteRedraw) {
    m_bDelayedCompleteRedraw = false;
    m_bCompleteRedraw = true;
    m_bRedraw = true;
  }

  return m_bRedraw;
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