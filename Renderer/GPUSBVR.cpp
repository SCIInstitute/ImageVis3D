#include "GPUSBVR.h"

#include <math.h>
#include <Basics/SysTools.h>
#include "../Controller/MasterController.h"

GPUSBVR::GPUSBVR(MasterController* pMasterController) :
	m_xRot(0),
	m_iCurrentView(0)
{
	m_pMasterController = pMasterController;



}

GPUSBVR::~GPUSBVR() {
}


void GPUSBVR::Initialize() {

	m_pMasterController->DebugOut()->Message("GPUSBVR::Initialize","");

	glClearColor(1,0,0,0);
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

void GPUSBVR::Paint() {
//	m_pMasterController->DebugOut()->Message("GPUSBVR::Paint","");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslated(0.0, 0.0, -10.0);
	glRotated(m_xRot / 16.0, 1.0, 0.0, 0.0);

	m_IDTex[m_iCurrentView]->Bind();

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
}

void GPUSBVR::Resize(int width, int height) {
	m_pMasterController->DebugOut()->Message("GPUSBVR::Resize","");

	int side = std::min(width, height);
	glViewport((width - side) / 2, (height - side) / 2, side, side);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0);
	glMatrixMode(GL_MODELVIEW);
}


void GPUSBVR::Cleanup() {
//	glDeleteLists(object, 1);
}
