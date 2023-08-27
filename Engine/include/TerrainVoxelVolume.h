#pragma once
#include "ITerrainVoxelVolume.h"
class TerrainVoxelVolume : public ITerrainVoxelVolume
{
public:
	void DebugVisualiseVolume();
	// Inherited via ITerrainVoxelVolume
	virtual ivec3 GetTerrainTotalDims() override;
private:
	u32 DimensionsInVoxels = 2048;
};