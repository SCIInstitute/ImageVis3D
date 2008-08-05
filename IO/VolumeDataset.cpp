#include "VolumeDataset.h"


VolumeDataset::VolumeDataset(const std::string& strFilename) : 
	m_strFilename(strFilename) 
{
}

bool VolumeDataset::IsLoaded() const
{
	return true;
}
