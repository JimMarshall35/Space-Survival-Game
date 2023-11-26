#pragma once
#include "CommonTypedefs.h"
#include "ITerrainOctree.h"
#include "IVoxelDataSource.h"
#include "ITerrainOctreeNode.h"
#include "OctreeTypes.h"
#include "Core.h"
#include <glm.hpp>
#include <vector>
#include "SparseTerrainVoxelOctree.h"
#include <thread>
#include <mutex>
using namespace glm;

class IAllocator;
class Camera;
class ITerrainPolygonizer;
class ITerrainGraphicsAPIAdaptor;
class ITerrainVoxelPopulator;

class APP_API SparseTerrainVoxelOctree : public IVoxelDataSource
{
public:

	struct SparseTerrainOctreeNode : public ITerrainOctreeNode
	{
		SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner, u32 sizeInVoxels);
		SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner);
		SparseTerrainOctreeNode* Children[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		u32 MipLevel;
		ivec3 BottomLeftCorner;
		TerrainChunkMesh Mesh;
		u32 SizeInVoxels;
		i8* VoxelData = nullptr; // only set when MipLevel = 0
		//std::mutex Mutex;
		virtual ITerrainOctreeNode* GetChild(u8 child)const override { return static_cast<ITerrainOctreeNode*>(Children[child]); }
		virtual const ivec3& GetBottomLeftCorner()const override { return BottomLeftCorner; }
		virtual u32 GetSizeInVoxels() const override { return SizeInVoxels; }
		virtual u32 GetMipLevel() const override { return MipLevel; }
		virtual const TerrainChunkMesh& GetTerrainChunkMesh() const override;
		virtual void SetTerrainChunkMesh(const TerrainChunkMesh& mesh) override;
		virtual bool NeedsRegenerating() const override { return Mesh.bNeedsRegenerating; }
		virtual TerrainChunkMesh& GetTerrainChunkMeshMutable() override { return Mesh; }
		virtual i8* GetVoxelData() override { return VoxelData; };
		virtual void SetVoxelData(i8* newData) { VoxelData = newData; }
	};

public:

	SparseTerrainVoxelOctree(IAllocator* allocator, ITerrainPolygonizer* polygonizer, ITerrainGraphicsAPIAdaptor* graphicsAPIAdaptor, u32 sizeVoxels, i8 clampValueHigh, i8 clampValueLow);

	SparseTerrainVoxelOctree(IAllocator* allocator, ITerrainPolygonizer* polygonizer, ITerrainGraphicsAPIAdaptor* graphicsAPIAdaptor, u32 sizeVoxels, i8 clampValueHigh, i8 clampValueLow, ITerrainVoxelPopulator* populator);


	~SparseTerrainVoxelOctree();

	

	//IVoxelDataSource

	virtual ITerrainOctreeNode* FindNodeFromIndex(TerrainOctreeIndex index, bool createIfDoesntExist) override;

	virtual void GetVoxelsForNode(ITerrainOctreeNode* node, i8* outVoxels) override;
	
	virtual i8 GetVoxelAt(const glm::ivec3& location) override;
	
	virtual TerrainOctreeIndex SetVoxelAt(const glm::ivec3& location, i8 value) override;

	virtual void Clear() override;

	virtual void ResizeAndClear(const size_t newSize) override;

	virtual size_t GetSize() const override;

	virtual void AllocateNodeVoxelData(ITerrainOctreeNode* node) override;

	virtual ITerrainOctreeNode* GetParentNode() override { return &ParentNode; };

	//IVoxelDataSource end

	// return a list of TerrainOctreeNodes to render.
	// these will be frustum culled and LOD'd correctly 
	// from the cam data provided
	void GetChunksToRender(
		const Camera& camera,
		float aspect, float fovY, float zNear, float zFar,
		std::vector<ITerrainOctreeNode*>& outNodesToRender);

private:

	TerrainOctreeIndex SetVoxelAt_Internal(const glm::ivec3& location, i8 value);

	/// <param name="allocateNewIfNull">if the point is in a branch that does not exist do we allocate it or return the default value</param>
	SparseTerrainOctreeNode* FindChildContainingPoint(SparseTerrainOctreeNode* parent, const glm::ivec3& point, u8& outChildIndex, bool allocateNewIfNull = true);

	void DeleteAllChildren(SparseTerrainOctreeNode* node);

	glm::ivec3 GetLocationWithinMipZeroCellFromWorldLocation(SparseTerrainOctreeNode* mipZeroCell, const glm::ivec3& globalLocation);

private:

	IAllocator* Allocator;

	SparseTerrainOctreeNode ParentNode;

	i8 VoxelClampValueHigh;

	i8 VoxelClampValueLow;

	i8 VoxelDefaultValue;

	// this has a bounded upper size so we can allocate it once on construction
	SparseTerrainOctreeNode** ParentNodeStack;

	ITerrainPolygonizer* Polygonizer;

	ITerrainGraphicsAPIAdaptor* GraphicsAPIAdaptor;

	std::mutex SetVoxelMutex;
};