#include "ITerrainVoxelPopulator.h"

class ITerrainVoxelPopulator;

class TestProceduralTerrainVoxelPopulator : public ITerrainVoxelPopulator
{
public:

	// Inherited via ITerrainVoxelPopulator
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) override;
};