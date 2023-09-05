#include "IVoxelDataSource.h"

class ITerrainVoxelPopulator
{
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) = 0;
};