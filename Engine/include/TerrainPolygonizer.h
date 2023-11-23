#pragma once
#include "ITerrainPolygonizer.h"
#include "ThreadPool.h"
#include "CommonTypedefs.h"
#include <glm.hpp>
#include <atomic>
#include <memory>

struct ITerrainOctreeNode;
class IAllocator;

class TerrainPolygonizer : public ITerrainPolygonizer
{
public:
	TerrainPolygonizer(IAllocator* allocator, std::shared_ptr<rdx::thread_pool> threadPool);
	~TerrainPolygonizer();
	// Inherited via ITerrainPolygonizer
	virtual std::future<PolygonizeWorkerThreadData*> PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source) override;
	PolygonizeWorkerThreadData* PolygonizeCellSync(ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source);
private:
	//typedef 
	

private:
	std::shared_ptr<rdx::thread_pool> ThreadPool;
	IAllocator* Allocator;
	std::atomic_int NumActiveWorkers = 0;
};