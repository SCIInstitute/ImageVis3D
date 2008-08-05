#include "MasterController.h"

MasterController::MasterController() {

}

MasterController::~MasterController() {
	for (size_t i = 0;i<m_vVolumeRenderer.size();i++) delete m_vVolumeRenderer[i];
	m_vVolumeRenderer.resize(0);
	for (size_t i = 0;i<m_v1DTrans.size();i++) delete m_v1DTrans[i];
	m_v1DTrans.resize(0);
	for (size_t i = 0;i<m_v2DTrans.size();i++) delete m_v2DTrans[i];
	m_v2DTrans.resize(0);
}


AbstrRenderer* MasterController::RequestNewVolumerenderer(VolumeRenderer eRendererType) {
	switch (eRendererType) {
		case OPENGL_SBVR : {
								m_vVolumeRenderer.push_back(new GPUSBVR(m_GPUMemMan));
								return m_vVolumeRenderer[m_vVolumeRenderer.size()-1];
						   }
		default : return NULL;
	};
}