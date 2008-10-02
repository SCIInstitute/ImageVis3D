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

#include <math.h>
#include <Basics/SysTools.h>
#include "../Controller/MasterController.h"

GPUSBVR::GPUSBVR(MasterController* pMasterController) :
  m_iCurrentView(0),
  m_vRot(0,0),
  m_bDelayedCompleteRedraw(false),
  m_bRenderWireframe(false)
{
  m_pMasterController = pMasterController;
}

GPUSBVR::~GPUSBVR() {
}


void GPUSBVR::Initialize() {

  m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","");

  glClearColor(0.2f,0.2f,0.2f,0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  
  m_IDTex[0] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin1x3.bmp").c_str());
  if (m_IDTex[0] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","First Image load failed");    
  }
  m_IDTex[1] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin2x2.bmp").c_str());
  if (m_IDTex[1] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","Second Image load failed");    
  }
  m_IDTex[2] = m_pMasterController->MemMan()->Load2DTextureFromFile(SysTools::GetFromResourceOnMac("RenderWin1.bmp").c_str());
  if (m_IDTex[2] == NULL) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","Third Image load failed");    
  }
}


void GPUSBVR::UpdateGeoGen(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick) {

  UINTVECTOR3  vSize = m_pDataset->GetBrickSize(vLOD, vBrick);
  FLOATVECTOR3 vCenter, vExtension;
  m_pDataset->GetBrickCenterAndExtension(vLOD, vBrick, vCenter, vExtension);

  // TODO: transfer brick offsets to m_SBVRGeogen
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

    if (m_IDTex[m_iCurrentView] != NULL) m_IDTex[m_iCurrentView]->Bind();
    glDisable(GL_TEXTURE_3D);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
      glColor4d(1,1,1,1);
      glTexCoord2d(0,0);
      glVertex3d(0.2,  0.4, -0.5);
      glTexCoord2d(1,0);
      glVertex3d( 0.4,  0.4, -0.5);
      glTexCoord2d(1,1);
      glVertex3d( 0.4, 0.2, -0.5);
      glTexCoord2d(0,1);
      glVertex3d(0.2, 0.2, -0.5);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void GPUSBVR::Paint() {
  if (true || m_bCompleteRedraw) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","Complete Redraw");

    // DEBUG: just render the smallest brick for a start
    std::vector<UINT64> vSmallestLOD = m_pDataset->GetInfo()->GetLODLevelCount();
    for (size_t i = 0;i<vSmallestLOD.size();i++) vSmallestLOD[i] -= 1;   
    std::vector<UINT64> vFirstBrick(m_pDataset->GetInfo()->GetBrickCount(vSmallestLOD).size());
    for (size_t i = 0;i<vFirstBrick.size();i++) vFirstBrick[i] = 0;
    UpdateGeoGen(vSmallestLOD, vFirstBrick);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawLogo();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    FLOATMATRIX4 trans, rotx, roty;
    trans.Translation(0,0,-4);
    rotx.RotationAxis(FLOATVECTOR3(0,1,0),m_vRot.x);
    roty.RotationAxis(FLOATVECTOR3(1,0,0),m_vRot.y);
    m_matModelView = trans*rotx*roty;

    //m_matModelView.setModelview();
    m_SBVRGeogen.SetTransformation(m_matModelView);

  	glEnable(GL_BLEND);
	  glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

    // switch to wireframe
    if (m_bRenderWireframe) {
      // TODO save state
	    glDisable(GL_CULL_FACE);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	    glEdgeFlag(true);
    }

    GLTexture3D* t = m_pMasterController->MemMan()->Get3DTexture(m_pDataset, vSmallestLOD, vFirstBrick);

    t->Bind();

    glEnable(GL_TEXTURE_3D);

    glBegin(GL_TRIANGLES);
      glColor4f(1,1,1,1);

      for (size_t i = 0;i<m_SBVRGeogen.m_vSliceTriangles.size();i++) {
        glTexCoord3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vTex);
        glVertex3fv(m_SBVRGeogen.m_vSliceTriangles[i].m_vPos);
      }
    glEnd();

    glDisable(GL_BLEND);

    if (m_bRenderWireframe) {
      // TODO: restore state
    }

  } else {
/*    m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","Quick Redraw");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslated(0.0, 0.0, -10.0);
    glRotated(m_xRot / 16.0, 1.0, 0.0, 0.0);

    if (m_IDTex[m_iCurrentView] != NULL) m_IDTex[m_iCurrentView]->Bind();

    glBegin(GL_QUADS);
      glColor4d(1,1,1,1);
      glTexCoord2d(0,0);
      glVertex3d(-0.5,  0.5, 0.0);
      glTexCoord2d(1,0);
      glVertex3d( 0.5,  0.5, 0.0);
      glTexCoord2d(1,1);
      glVertex3d( 0.5, -0.5, 0.0);
      glTexCoord2d(0,1);
      glVertex3d(-0.5, -0.5, 0.0);
    glEnd();
    */
  }

  m_bCompleteRedraw = false;
  m_bRedraw = false;
}

void GPUSBVR::Resize(int width, int height) {
  m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","");

//  int side = std::min(width, height);
//  glViewport((width - side) / 2, (height - side) / 2, side, side);

  float aspect=(float)width/(float)height;
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);		
	glLoadIdentity();
	gluPerspective(50.0,aspect,0.2,100.0); 	// Set Projection. Arguments are FOV (in degrees), aspect-ratio, near-plane, far-plane.
	glMatrixMode(GL_MODELVIEW);

  m_pFBO3DImage = new GLFBOTex(m_pMasterController, GL_NEAREST, GL_NEAREST, GL_CLAMP, width, height, GL_RGBA8, 8*4);

  m_bDelayedCompleteRedraw = true;
}

void GPUSBVR::Cleanup() {
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[0]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[1]);
  m_pMasterController->MemMan()->FreeTexture(m_IDTex[2]);

  delete m_pFBO3DImage;
}


void GPUSBVR::Set1DTrans(TransferFunction1D* p1DTrans) {
  AbstrRenderer::Set1DTrans(p1DTrans);
  m_bRedraw = true;
  m_bCompleteRedraw = true;
}

void GPUSBVR::Set2DTrans(TransferFunction2D* p2DTrans) {
  AbstrRenderer::Set2DTrans(p2DTrans);
  m_bRedraw = true;
  m_bCompleteRedraw = true;
}

void GPUSBVR::Changed1DTrans() {
  if (m_eRenderMode != RM_1DTRANS) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Changed1DTrans","not using the 1D transferfunction at the moment, ignoring message");
  } else {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Changed1DTrans","complete redraw scheduled");
    m_bRedraw = true;
    m_bCompleteRedraw = true;
  }
}

void GPUSBVR::Changed2DTrans() {
  if (m_eRenderMode != RM_2DTRANS) {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Changed2DTrans","not using the 2D transferfunction at the moment, ignoring message");
  } else {
    m_pMasterController->DebugOut()->Message("GPUSBVR::Changed2DTrans","complete redraw scheduled");
    m_bRedraw = true;
    m_bCompleteRedraw = true;
  }
}


void GPUSBVR::CheckForRedraw() {
  if (m_bDelayedCompleteRedraw) {
    m_bDelayedCompleteRedraw = false;
    m_bCompleteRedraw = true;
    m_bRedraw = true;
  }

  if (m_bRedraw) Paint();
}
