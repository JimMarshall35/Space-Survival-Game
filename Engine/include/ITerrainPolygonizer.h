#pragma once
#include <functional>
#include <future>
#include <glm.hpp>
#include "CommonTypedefs.h"
#include "Core.h"

struct ITerrainOctreeNode;
struct Triangle;
class IVoxelDataSource;
class IAllocator;

typedef glm::vec3 TerrainPosition;
typedef glm::vec3 TerrainNormal;

typedef glm::uvec3 TerrainPositionFixed;
typedef glm::uvec3 TerrainNormalFixed;

static_assert(sizeof(glm::vec3) == sizeof(glm::ivec3));

struct APP_API TerrainVertex
{
	TerrainPosition Position;
	TerrainNormal Normal;
};

struct APP_API TerrainVertexFixedPoint
{
	TerrainPositionFixed Position;
	TerrainNormalFixed Normal;
};

struct APP_API TerrainTransitionMeshGeometry
{
	u32* Indices;
	TerrainVertex* Vertices;
	u32 OutputtedVertices;
	u32 OutputtedIndices;
};

struct APP_API PolygonizeWorkerThreadData
{
	TerrainVertex* Vertices;
	u32* Indices;
	Triangle* Tris;
	u32 VerticesSize;
	u32 IndicesSize;
	u32 OutputtedVertices;
	u32 OutputtedIndices;
	std::mutex Mutex;
	i8* VoxelData;
	ITerrainOctreeNode* Node;
	IAllocator* MyAllocator;
	TerrainTransitionMeshGeometry TransitionMeshes[6];
	void* GetPtrToDeallocate() { return this; } // we allocate all data, positions, normals, ect in one big block starting with the PolygonizeWorkerThreadData itself
};

class APP_API ITerrainPolygonizer
{
public:
	virtual std::future<PolygonizeWorkerThreadData*> PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source) = 0;
};