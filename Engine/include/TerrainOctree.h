#pragma once
#include "CommonTypedefs.h"
#include "ITerrainOctree.h"
#include <memory>
#include <glm.hpp>
#include <vector>
#include "Camera.h"

using namespace glm;

class ITerrainVoxelVolume;
class IAllocator;
class Camera;
struct Frustum;

typedef u64 TerrainOctreeIndex;

struct TerrainChunkMesh
{
	u32 VAO = 0xffffffff;
	u32 VBO = 0xffffffff;
	bool bNeedsRegenerating;
};

struct ITerrainOctreeNode
{
	virtual ITerrainOctreeNode* GetChild(u8 child) const = 0;
	virtual const ivec3& GetBottomLeftCorner() const = 0;
	virtual u32 GetSizeInVoxels() const = 0;
	virtual u32 GetMipLevel() const = 0;
};

class TerrainLODSelectionAndCullingAlgorithm
{
public:
	TerrainLODSelectionAndCullingAlgorithm(float minimumViewportAreaThreshold = 1.0);
	void GetChunksToRender(
		const Frustum& frustum,
		std::vector<ITerrainOctreeNode*>& outNodesToRender,
		ITerrainOctreeNode* onNode,
		const glm::mat4& viewProjectionMatrix);

	// estimate how much space a block will take up in the view port
	float ViewportAreaHeuristic(ITerrainOctreeNode* block, const glm::mat4& viewProjectionMatrix);

	// if the viewport area heuristic for a block is < this value then
	// the block will be rendered and it's subtree skipped.
	float MinimumViewportAreaThreshold = 1.0;
};

class OctreeFunctionLibrary
{
public:
	// is a number a power of 2
	static bool IsAPowerOfTwo(i32 number);

	// a block width of 16 by 16 by 16 voxels is mip level 0 
	// and doubling these dims is the next mip level. Find level of
	// the entire cube which is already asserted to be a power of 2.
	static u32 GetMipLevel(i32 cellVoxelsWidth);

	static u32 GetSizeInVoxels(u32 mipLevel);

	static bool IsPointInCube(const glm::ivec3& point, const glm::ivec3& cubeBottomLeft, u32 cubeSizeVoxels);
};

class SparseTerrainVoxelOctree : public ITerrainOctree
{
public:

	struct SparseTerrainOctreeNode
	{
		SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner, u32 sizeInVoxels);
		SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner);
		SparseTerrainOctreeNode* Children[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		u32 MipLevel;
		ivec3 BottomLeftCorner;
		TerrainChunkMesh Mesh;
		u32 SizeInVoxels;
		i8* VoxelData = nullptr; // only set when MipLevel = 0
	};

public:

	SparseTerrainVoxelOctree(IAllocator* allocator, u32 sizeVoxels, i8 clampValueHigh, i8 clampValueLow);
	
	~SparseTerrainVoxelOctree();

	TerrainOctreeIndex SetVoxelAt(const glm::ivec3& location, i8 value);

	i8 GetVoxelAt(const glm::ivec3& location);

	SparseTerrainOctreeNode* FindNodeFromIndex(TerrainOctreeIndex index);

	void FillPrePolygonizingVoxelStagingArea(SparseTerrainOctreeNode* node, i8* outVoxels);

private:

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

};

class DebugVisualizerTerrainOctree : public ITerrainOctree
{
private:

	struct TerrainOctreeNode : public ITerrainOctreeNode
	{
		std::unique_ptr<TerrainOctreeNode> Children[8];
		u32 MipLevel;
		ivec3 BottomLeftCorner;
		u32 SizeInVoxels;
		virtual ITerrainOctreeNode* GetChild(u8 child)const override { return Children[child].get(); }
		virtual const ivec3& GetBottomLeftCorner()const override { return BottomLeftCorner; }
		virtual u32 GetSizeInVoxels() const override { return SizeInVoxels; }
		virtual u32 GetMipLevel() const override { return MipLevel; }
	};

public:

	DebugVisualizerTerrainOctree(ITerrainVoxelVolume* volume);

	// initialise octree
	void MakeQuadtree();

	// return a list of TerrainOctreeNodes to render.
	// these will be frustum culled and LOD'd correctly 
	// from the cam data provided
	void GetChunksToRender(
		const Camera& camera,
		float aspect, float fovY, float zNear, float zFar,
		std::vector<ITerrainOctreeNode*>& outNodesToRender);

	// draw a gizmo box around each chunk in list
	void DebugVisualiseChunks(const std::vector<ITerrainOctreeNode*>& nodes);

	bool bDebugFill = false;

	i32 DebugMipLevelToDraw = -1;

	f32 GetMinimumViewportAreaThreshold() { return LODCulling.MinimumViewportAreaThreshold; }

	void SetMinimumViewportAreaThreshold(f32 val) { LODCulling.MinimumViewportAreaThreshold = val; }

private:

	// recursive function called from MakeQuadtree
	void PopulateChildren(TerrainOctreeNode* node);

	void PopulateDebugMipLevelColourTable(u32 maximumMipLevel);

	void DebugVisualiseMipLevel(TerrainOctreeNode* parentNode, u32 mipLevel, const glm::vec4& colour);

private:
	
	TerrainLODSelectionAndCullingAlgorithm LODCulling;

	// parent node of the octree representing the entire voxel volume.
	// children are chunks of half size, one in each quadrant of their parent.
	TerrainOctreeNode ParentNode;

	// the source of voxel data.
	ITerrainVoxelVolume* Volume;

	// for colouring different mip level blocks different colours during debugging
	std::unique_ptr<glm::vec4[]> DebugMipLevelColourTable;

	bool bDebugCameraSet;
	
	Camera DebugCamera;
};