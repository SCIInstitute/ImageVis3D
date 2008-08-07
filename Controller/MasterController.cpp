#include "MasterController.h"

MasterController::MasterController() :
	m_pDebugOut(new ConsoleOut()),
	m_bStartDebugOut(true)
{
	m_pGPUMemMan = new GPUMemMan(this);
}

MasterController::~MasterController() {
	for (AbstrRendererListIter i = m_vVolumeRenderer.begin();i<m_vVolumeRenderer.end();i++) delete (*i);
	m_vVolumeRenderer.resize(0);

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


void MasterController::ReleaseVolumerenderer(AbstrRenderer* pVolumeRenderer) {

	for (AbstrRendererListIter i = m_vVolumeRenderer.begin();i<m_vVolumeRenderer.end();i++) {
		if (*i == pVolumeRenderer) {
			m_pDebugOut->Message("MasterController::ReleaseVolumerenderer","Deleting volume renderer");
			delete pVolumeRenderer;
			m_vVolumeRenderer.erase(i);
			return;
		}
	}
	m_pDebugOut->Warning("MasterController::ReleaseVolumerenderer","requested volume rendere not found");
}
