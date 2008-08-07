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
	if (m_bStartDebugOut) delete m_pDebugOut;
}

void MasterController::SetDebugOut(AbstrDebugOut* debugOut) {
	if (debugOut != NULL) {
		m_pDebugOut->Message("MasterController::SetDebugOut","Disconnecting from this debug out");
		if (m_bStartDebugOut ) {
			delete m_pDebugOut;
			m_bStartDebugOut = false;
		}
		m_pDebugOut = debugOut;
		m_pDebugOut->Message("MasterController::SetDebugOut","Connected to this debug out");
	}
}

void MasterController::RemoveDebugOut(AbstrDebugOut* debugOut) {
	if (debugOut == m_pDebugOut) {
		m_pDebugOut->Message("MasterController::RemoveDebugOut","Disconnecting from this debug out");
		if (m_bStartDebugOut) delete m_pDebugOut;
		m_pDebugOut = new ConsoleOut();
		m_bStartDebugOut = true;
		m_pDebugOut->Message("MasterController::RemoveDebugOut","Connected to this debug out");
	} else {
		m_pDebugOut->Warning("MasterController::RemoveDebugOut","Not Connected the debug out in question (anymore), doing nothing");
	}
}


AbstrRenderer* MasterController::RequestNewVolumerenderer(VolumeRenderer eRendererType) {

	m_pDebugOut->Message("MasterController::RequestNewVolumerenderer","");

	switch (eRendererType) {
		case OPENGL_SBVR : {
								m_vVolumeRenderer.push_back(new GPUSBVR(this));
								return m_vVolumeRenderer[m_vVolumeRenderer.size()-1];
						   }
		default : return NULL;
	};
}
