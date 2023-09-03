#include "TerrainPolygonizer.h"
#include "IAllocator.h"
#include "TerrainDefs.h"
#include "IVoxelDataSource.h"

#define TERRAIN_CELL_VERTEX_ARRAY_SIZE 5000 // each worker can output this number of vertices maximum
#define TERRAIN_CELL_INDEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum

TerrainPolygonizer::TerrainPolygonizer(IAllocator* allocator)
	:Allocator(allocator),
	ThreadPool(std::thread::hardware_concurrency())
{
	size_t threadPoolSize = std::thread::hardware_concurrency();
}

TerrainPolygonizer::~TerrainPolygonizer()
{
	
}

std::future<PolygonizeWorkerThreadData*> TerrainPolygonizer::PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source)
{
	//auto r = ThreadPool.enqueue(PolygonizeCellSync, node);
	auto r = ThreadPool.enqueue([this, node, source]() {
		return PolygonizeCellSync(node, source);
	});
	return r;
}

PolygonizeWorkerThreadData* TerrainPolygonizer::PolygonizeCellSync(ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source)
{
	u8* data = (u8*)Allocator->Malloc(
		sizeof(PolygonizeWorkerThreadData) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainNormal) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainPosition) +
		TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32) +
		TOTAL_CELL_VOLUME_SIZE * sizeof(i8)
	); // malloc everything in a single block

	PolygonizeWorkerThreadData* rVal = (PolygonizeWorkerThreadData*)data;
	u8* dataPtr = data + sizeof(PolygonizeWorkerThreadData);


	rVal->Normals = (TerrainNormal*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainNormal);
	rVal->Positions = (TerrainPosition*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainPosition);
	rVal->Indices = (u32*)dataPtr;
	dataPtr += TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32);
	rVal->VoxelData = (i8*)dataPtr;

	rVal->VerticesSize = TERRAIN_CELL_VERTEX_ARRAY_SIZE;
	rVal->IndicesSize = TERRAIN_CELL_INDEX_ARRAY_SIZE;
	rVal->Node = cellToPolygonize;
	rVal->OutputtedVertices = 0;
	rVal->OutputtedIndices = 0;
	rVal->MyAllocator = Allocator;

	source->GetVoxelsForNode(cellToPolygonize, rVal->VoxelData);

	// create polygons from voxel data here:
	
	return rVal;
}
