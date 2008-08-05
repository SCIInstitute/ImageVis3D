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


VolumeDataset* GPUMemMan::LoadDataset(const std::string& strFilename) {
	for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
		if (i->second->Filename() == strFilename) {
			m_MasterController->DebugOut()->printf("GPUMemMan::LoadDataset: Reusing %s", strFilename.c_str());
			i->first++;
			return i->second;
		}
	}

	m_MasterController->DebugOut()->printf("GPUMemMan::LoadDataset: Loading %s", strFilename.c_str());
	VolumeDataset* dataset = new VolumeDataset(strFilename);
	if (dataset->IsLoaded()) {
		m_vpVolumeDatasets.push_back(VolDataListElem(1,dataset));
		return dataset;
	} else return NULL;
}


void GPUMemMan::FreeDataset(VolumeDataset* pVolumeDataset) {
	for (VolDataListIter i = m_vpVolumeDatasets.begin();i<m_vpVolumeDatasets.end();i++) {
		if (i->second == pVolumeDataset) {
			i->first--;
			if (i->first == 0) {
				delete pVolumeDataset;
				m_vpVolumeDatasets.erase(i);
			}
			break;
		}
	}
}

GLuint GPUMemMan::Load2DTextureFromFile(const std::string& strFilename) {
	for (SimpleTextureListIter i = m_vpSimpleTextures.begin();i<m_vpSimpleTextures.end();i++) {
		if (i->strFilename == strFilename) {
			m_MasterController->DebugOut()->printf("GPUMemMan::Load2DTextureFromFile: Reusing %s", strFilename.c_str());
			i->iAccessCounter++;
			return i->iGLID;
		}
	}

	TextureImage image;
	if (!BMPLoader::Load(strFilename, &image)) {
		m_MasterController->DebugOut()->printf("GPUMemMan::Load2DTextureFromFile: Unable to load file %s", strFilename.c_str());
		return -1;
	}
	m_MasterController->DebugOut()->printf("GPUMemMan::Load2DTextureFromFile: Loading %s", strFilename.c_str());

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
