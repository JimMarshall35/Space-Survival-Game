#pragma once
#include "gmock/gmock.h"
#include "ITerrainPolygonizer.h"
#include "IVoxelDataSource.h"
#include "ITerrainOctreeNode.h"
#include "ITerrainGraphicsAPIAdaptor.h"

class MockTerrainGraphicsAPIAdaptor : public ITerrainGraphicsAPIAdaptor
{
public:
	MOCK_METHOD(void, UploadNewlyPolygonizedToGPU, (PolygonizeWorkerThreadData* data), (override));
	MOCK_METHOD(void, RenderTerrainNodes, (const std::vector<ITerrainOctreeNode*>& nodes, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection), (override));
	MOCK_METHOD(void, SetTerrainMaterial, (const TerrainMaterial& material), (override));
	MOCK_METHOD(void, SetTerrainLight, (const TerrainLight& light), (override));
};

struct MockTerrainOctreeNode : public ITerrainOctreeNode
{
public:
	MOCK_METHOD(ITerrainOctreeNode*, GetChild, (u8 child), (const, override));
	MOCK_METHOD(const glm::ivec3&, GetBottomLeftCorner, (), (const, override));
	MOCK_METHOD(u32, GetSizeInVoxels, (), (const, override));
	MOCK_METHOD(u32, GetMipLevel, (), (const, override));
	MOCK_METHOD(const TerrainChunkMesh&, GetTerrainChunkMesh, (), (const, override));
	MOCK_METHOD(TerrainChunkMesh&, GetTerrainChunkMeshMutable, (), (override));
	MOCK_METHOD(void, SetTerrainChunkMesh, (const TerrainChunkMesh& mesh), (override));
	MOCK_METHOD(bool, NeedsRegenerating, (), (const, override));

};

class MockVoxelDataSource : public IVoxelDataSource
{
public:
	MOCK_METHOD(i8, GetVoxelAt, (const glm::ivec3& valueAt), (override));
	MOCK_METHOD(void, GetVoxelsForNode, (ITerrainOctreeNode* node, i8* outVoxels), (override));
	MOCK_METHOD(void, SetVoxelAt, (const glm::ivec3& location, i8 value), (override));
	MOCK_METHOD(void, Clear, (), (override));
	MOCK_METHOD(void, ResizeAndClear, (const size_t newSize), (override));
	MOCK_METHOD(size_t, GetSize, (), (const, override));

};

class MockPolygonizer : public ITerrainPolygonizer
{
public:
	MOCK_METHOD(std::future<PolygonizeWorkerThreadData*>, PolygonizeNodeAsync, (ITerrainOctreeNode* node, IVoxelDataSource* source), (override));
};