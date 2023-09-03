#pragma once
#include <functional>
#include <future>
#include <glm.hpp>
#include "CommonTypedefs.h"

struct ITerrainOctreeNode;
class IVoxelDataSource;
class IAllocator;

typedef glm::vec3 TerrainPosition;
typedef glm::vec3 TerrainNormal;

struct PolygonizeWorkerThreadData
{
	TerrainPosition* Positions;
	TerrainNormal* Normals;
	u32* Indices;
	u32 VerticesSize;
	u32 IndicesSize;
	u32 OutputtedVertices;
	u32 OutputtedIndices;
	std::mutex Mutex;
	i8* VoxelData;
	ITerrainOctreeNode* Node;
	IAllocator* MyAllocator;
	void* GetPtrToDeallocate() { return this; } // we allocate all data, positions, normals, ect in one big block starting with the PolygonizeWorkerThreadData itself
};

class ITerrainPolygonizer
{
public:
	virtual std::future<PolygonizeWorkerThreadData*> PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source) = 0;
};