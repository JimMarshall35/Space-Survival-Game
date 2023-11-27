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
public:
	bool bExactFit = false;
private:
	struct GridCell
	{
		float val[8];
		glm::vec3 p[8];
		glm::ivec3 coords[8];
	};
private:
	TerrainVertex VertexInterp( glm::vec3 p1, glm::vec3 p2,float valp1,float valp2, glm::ivec3& coords1, glm::ivec3& coords2, i8* voxels, ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source);
	int Polygonise(GridCell &Grid, int &NewVertexCount, TerrainVertex *Vertices, int& newIndicesCount, char* indices, i8* voxels, ITerrainOctreeNode* node, IVoxelDataSource* source);

private:
	std::shared_ptr<rdx::thread_pool> ThreadPool;
	IAllocator* Allocator;
	std::atomic_int NumActiveWorkers = 0;
};