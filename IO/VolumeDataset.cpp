#include "VolumeDataset.h"


VolumeDataset::VolumeDataset(const std::string& strFilename) : 
	m_strFilename(strFilename),
	m_pVolumeDatasetInfo(NULL),
	m_pHist1D(NULL), 
	m_pHist2D(NULL)
{
	LoadDataset();
	ComputeHistogramms();
}

VolumeDataset::~VolumeDataset()
{
	delete m_pHist1D;
	delete m_pHist2D;
	delete m_pVolumeDatasetInfo;
}


bool VolumeDataset::IsLoaded() const
{
	// TODO
	return true;
}


void VolumeDataset::LoadDataset() 
{
	// TODO
	m_pVolumeDatasetInfo = new VolumeDatasetInfo(8);
}

void VolumeDataset::ComputeHistogramms() 
{
	// DEBUG CODE
	// generate a random 1D histogram
	m_pHist1D = new Histogram1D(1<<m_pVolumeDatasetInfo->GetBitwith());
	for (size_t i = 0;i<m_pHist1D->GetSize();i++) m_pHist1D->Set(i, (unsigned int)(i+(rand()*10) / RAND_MAX));
	// generate a random 2D histogram
	m_pHist2D = new Histogram2D(VECTOR2<size_t>(1<<m_pVolumeDatasetInfo->GetBitwith(),256));  // TODO: decide: always 8bit gradient ?
	for (size_t y = 0;y<m_pHist2D->GetSize().y;y++)
		for (size_t x = 0;x<m_pHist2D->GetSize().x;x++) 
			m_pHist2D->Set(x,y,(unsigned int)(x+(rand()*10) / RAND_MAX)*(unsigned int)(y+(rand()*10) / RAND_MAX));
	// END DEBUG CODE
}
