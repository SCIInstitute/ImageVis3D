#include "GPUMemMan.h"
#include "../IO/Images/BMPLoader.h"
#include "../Controller/MasterController.h"

using namespace std;

GPUMemMan::GPUMemMan(MasterController* masterController) :
	m_MasterController(masterController)
{

}

GPUMemMan::~GPUMemMan() {

}


VolumeDataset* GPUMemMan::LoadDataset(const std::string& strFilename, AbstrRenderer* requester) {
	for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
		if (i->pVolumeDataset->Filename() == strFilename) {
			m_MasterController->DebugOut()->Message("GPUMemMan::LoadDataset","Reusing %s", strFilename.c_str());
			i->qpUser.push_back(requester);
			return i->pVolumeDataset;
		}
	}

	m_MasterController->DebugOut()->Message("GPUMemMan::LoadDataset","Loading %s", strFilename.c_str());
	VolumeDataset* dataset = new VolumeDataset(strFilename);
	if (dataset->IsLoaded()) {
		m_vpVolumeDatasets.push_back(VolDataListElem(dataset, requester));
		return dataset;
	} else return NULL;
}


void GPUMemMan::FreeDataset(VolumeDataset* pVolumeDataset, AbstrRenderer* requester) {
	for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
		if (i->pVolumeDataset == pVolumeDataset) {
			for (AbstrRendererListIter j = i->qpUser.begin();j<i->qpUser.end();j++) {
				if (*j == requester) {
					i->qpUser.erase(j);
					if (i->qpUser.size() == 0) {
						m_MasterController->DebugOut()->Message("GPUMemMan::FreeDataset","Released Dataset %s", pVolumeDataset->Filename().c_str());
						delete pVolumeDataset;
						m_vpVolumeDatasets.erase(i);
					} else {
						m_MasterController->DebugOut()->Message("GPUMemMan::FreeDataset","Decreased access count but dataset %s is still in use by another subsystem", pVolumeDataset->Filename().c_str());
					}
					return;
				}
				break;
			}
		}
	}
	m_MasterController->DebugOut()->Warning("GPUMemMan::FreeDataset","Dataset %s not found or not being used by requester", pVolumeDataset->Filename().c_str());
}

GLuint GPUMemMan::Load2DTextureFromFile(const std::string& strFilename) {
	for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
		if (i->strFilename == strFilename) {
			m_MasterController->DebugOut()->Message("GPUMemMan::Load2DTextureFromFile","Reusing %s", strFilename.c_str());
			i->iAccessCounter++;
			return i->iGLID;
		}
	}

	TextureImage image;
	if (!BMPLoader::Load(strFilename, &image)) {
		m_MasterController->DebugOut()->Error("GPUMemMan::Load2DTextureFromFile","Unable to load file %s", strFilename.c_str());
		return -1;
	}
	m_MasterController->DebugOut()->Message("GPUMemMan::Load2DTextureFromFile","Loading %s", strFilename.c_str());

	GLuint iGLID;
	glGenTextures(1,&iGLID);
    glBindTexture(GL_TEXTURE_2D,iGLID);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,image.width,image.height,0, GL_RGB, GL_UNSIGNED_BYTE, image.data);

	delete [] image.data;

	m_vpSimpleTextures.push_back(SimpleTextureListElem(1,iGLID,strFilename));
	return iGLID;
}


void GPUMemMan::FreeTexture(GLuint iTexture) {
	for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
		if (i->iGLID == iTexture) {
			i->iAccessCounter--;
			if (i->iAccessCounter == 0) {
				glDeleteTextures(1,&iTexture);
				m_vpSimpleTextures.erase(i);
			}
			break;
		}
	}
}


void GPUMemMan::GetEmpty1DTrans(size_t iSize, AbstrRenderer* requester, TransferFunction1D** transferFunc, GLuint* iGLID) {
	m_MasterController->DebugOut()->Message("GPUMemMan::GetEmpty1DTrans","Creating new empty 1D transfer function");
	*transferFunc = new TransferFunction1D(iSize);


	// TODO create 1D texture of size iSize

	m_vpTrans1DList.push_back(Trans1DListElem(dataset, requester));
}

void GPUMemMan::Get1DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction1D** transferFunc, GLuint* iGLID) {

}

bool GPUMemMan::Access1DTrans(GLuint iGLID, AbstrRenderer* requester) {

}

void GPUMemMan::Free1DTrans(GLuint iGLID, AbstrRenderer* requester) {

}

void GPUMemMan::GetEmpty2DTrans(const VECTOR2<size_t>& iSize, AbstrRenderer* requester, TransferFunction2D** transferFunc, GLuint* iGLID) {

}

void GPUMemMan::Get2DTransFromFile(const std::string& strFilename, AbstrRenderer* requester, TransferFunction2D** transferFunc, GLuint* iGLID) {

}

bool GPUMemMan::Access2DTrans(GLuint iGLID, AbstrRenderer* requester) {

}

void GPUMemMan::Free2DTrans(GLuint iGLID, AbstrRenderer* requester) {

}

