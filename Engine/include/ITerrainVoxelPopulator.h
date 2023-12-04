#pragma once
#include "Core.h"
class IVoxelDataSource;

class APP_API ITerrainVoxelPopulator
{
public:
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) = 0;
};