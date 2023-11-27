#pragma once
#include <glm.hpp>
#include "CommonTypedefs.h"
#include "Core.h"
#include "OctreeTypes.h"
struct ITerrainOctreeNode;

class APP_API IVoxelDataSource
{
public:
	// need to make both getters const but can't due to FindChildContainingPoint implementation
	// TODO: sort out
	virtual i8 GetVoxelAt(const glm::ivec3& valueAt) = 0;
	virtual void GetVoxelsForNode(ITerrainOctreeNode* node, i8* outVoxels) = 0;
	virtual TerrainOctreeIndex SetVoxelAt(const glm::ivec3& location, i8 value) = 0;
	virtual void Clear() = 0;
	virtual void ResizeAndClear(const size_t newSize) = 0;
	virtual size_t GetSize() const = 0;
	virtual ITerrainOctreeNode* FindNodeFromIndex(TerrainOctreeIndex index, bool createIfDoesntExist = false) = 0;
	virtual void AllocateNodeVoxelData(ITerrainOctreeNode* node) = 0;
	virtual ITerrainOctreeNode* GetParentNode() = 0;
	virtual void CreateChildrenForFirstNMipLevels(ITerrainOctreeNode* node, int n, int onLevel=0) = 0;
};