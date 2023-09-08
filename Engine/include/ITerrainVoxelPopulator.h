#pragma once
class IVoxelDataSource;

class ITerrainVoxelPopulator
{
public:
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) = 0;
};