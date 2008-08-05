#include "AbstrRenderer.h"
#include "../Controller/MasterController.h"

using namespace std;

bool AbstrRenderer::LoadDataset(const string& strFilename) {
	if (m_pMasterController == NULL) return false;
	if (m_pMasterController->MemMan() == NULL) {
		m_pMasterController->DebugOut()->printf("AbstrRenderer::LoadDataset: Cannont load dataset because m_pMasterController->MemMan() == NULL");
		return false;
	}

	m_pDataset = m_pMasterController->MemMan()->LoadDataset(strFilename);

	if (m_pDataset == NULL) {
		m_pMasterController->DebugOut()->printf("AbstrRenderer::LoadDataset: MemMan call to load dataset failed");
		return false;
	}

	return true;
}

AbstrRenderer::~AbstrRenderer() {
	m_pMasterController->MemMan()->FreeDataset(m_pDataset);
}