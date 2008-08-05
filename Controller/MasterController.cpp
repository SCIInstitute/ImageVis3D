#include "MasterController.h"

MasterController::MasterController() :
	m_pDebugOut(new ConsoleOut()),
	m_bStartDebugOut(true)
{
	m_pGPUMemMan = new GPUMemMan(this);
}

MasterController::~MasterController() {
	for (size_t i = 0;i<m_vVolumeRenderer.size();i++) delete m_vVolumeRenderer[i];
	m_vVolumeRenderer.resize(0);
	for (size_t i = 0;i<m_v1DTrans.size();i++) delete m_v1DTrans[i];
	m_v1DTrans.resize(0);
	for (size_t i = 0;i<m_v2DTrans.size();i++) delete m_v2DTrans[i];
	m_v2DTrans.resize(0);

	delete m_pGPUMemMan;
}

void MasterController::SetDebugOut(AbstrDebugOut* debugOut) {
	if (debugOut != NULL) {
		m_pDebugOut->printf("MasterController::SetDebugOut: Disconnecting from debug out");
		if (m_bStartDebugOut ) {
			delete m_pDebugOut;
			m_bStartDebugOut = false;
		}
		m_pDebugOut = debugOut;
		m_pDebugOut->printf("MasterController::SetDebugOut: Connected to debug out");
	}
}

AbstrRenderer* MasterController::RequestNewVolumerenderer(VolumeRenderer eRendererType) {

	m_pDebugOut->printf("MasterController::RequestNewVolumerenderer");

	switch (eRendererType) {
		case OPENGL_SBVR : {
								m_vVolumeRenderer.push_back(new GPUSBVR(this));
								return m_vVolumeRenderer[m_vVolumeRenderer.size()-1];
						   }
		default : return NULL;
	};
}