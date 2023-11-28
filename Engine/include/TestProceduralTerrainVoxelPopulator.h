#pragma once
#include "ITerrainVoxelPopulator.h"
#include <memory>
#include <vector>
#include <future>
#include <unordered_set>
#include "OctreeTypes.h"

class ITerrainVoxelPopulator;
class ITerrainOctreeNode;
class SimplexNoise;

namespace rdx
{
	class thread_pool;
}

class TestProceduralTerrainVoxelPopulator : public ITerrainVoxelPopulator
{
public:
	TestProceduralTerrainVoxelPopulator(const std::shared_ptr<rdx::thread_pool>& threadPool);
	// Inherited via ITerrainVoxelPopulator
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) override;
private:
	void PopulateSingleNode(IVoxelDataSource* dataSrcToWriteTo, ITerrainOctreeNode* node, SimplexNoise& noise, std::unordered_set<TerrainOctreeIndex>* output);
private:
	std::shared_ptr<rdx::thread_pool> ThreadPool;
};