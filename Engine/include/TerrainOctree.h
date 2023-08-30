#pragma once
#include "CommonTypedefs.h"
#include "ITerrainOctree.h"
#include <memory>
#include <glm.hpp>
#include <vector>
#include "Camera.h"

using namespace glm;

class ITerrainVoxelVolume;
class Camera;
struct Frustum;

struct TerrainChunkMesh
{
	u32 VAO;
	u32 VBO;
};

struct TerrainOctreeNode
{
	std::unique_ptr<TerrainOctreeNode> Children[8];
	u32 MipLevel;
	ivec3 BottomLeftCorner;
	TerrainChunkMesh Mesh;
	u32 SizeInVoxels;
};

class TerrainOctree : public ITerrainOctree
{
public:
	TerrainOctree(ITerrainVoxelVolume* volume);

	// initialise octree
	void MakeQuadtree();

	// return a list of TerrainOctreeNodes to render.
	// these will be frustum culled and LOD'd correctly 
	// from the cam data provided
	void GetChunksToRender(
		const Camera& camera,
		float aspect, float fovY, float zNear, float zFar, 
		std::vector<TerrainOctreeNode*>& outNodesToRender);

	// draw a gizmo box around each chunk in list
	void DebugVisualiseChunks(const std::vector<TerrainOctreeNode*>& nodes);

	// if the viewport area heuristic for a block is < this value then
	// the block will be rendered and it's subtree skipped.
	float MinimumViewportAreaThreshold = 1.0;

	bool bDebugFill = false;

	i32 DebugMipLevelToDraw = -1;
private:

	void GetChunksToRender(
		const Frustum& frustum, 
		std::vector<TerrainOctreeNode*>& outNodesToRender, 
		TerrainOctreeNode* onNode, 
		const glm::mat4& viewProjectionMatrix);

	// estimate how much space a block will take up in the view port
	float ViewportAreaHeuristic(TerrainOctreeNode* block, const glm::mat4& viewProjectionMatrix);
	
	// recursive function called from MakeQuadtree
	void PopulateChildren(TerrainOctreeNode* node);
	
	// is a number a power of 2
	bool IsAPowerOfTwo(i32 number);
	
	// a block width of 16 by 16 by 16 voxels is mip level 0 
	// and doubling these dims is the next mip level. Find level of
	// the entire cube which is already asserted to be a power of 2.
	u32 GetMipLevel(i32 cellVoxelsWidth);

	void PopulateDebugMipLevelColourTable(u32 maximumMipLevel);

	void DebugVisualiseMipLevel(TerrainOctreeNode* parentNode, u32 mipLevel, const glm::vec4& colour);

private:
	

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