#include "TerrainOctree.h"
#include "ITerrainVoxelVolume.h"
#include <cassert>
#include <cmath>
#include "CameraFunctionLibrary.h"
#include <ext.hpp>
#define BASE_CELL_SIZE 16 // the size of all cells. lowest mip level is sampled 16x16x16 in actual dimensions with a cell size of 2, next level is 32x32x32 ect with a cell size of 4, ect. 
#include "Gizmos.h"

TerrainOctree::TerrainOctree(ITerrainVoxelVolume* volume)
	:Volume(volume)
{
	MakeQuadtree();
}

void TerrainOctree::MakeQuadtree()
{
	using namespace glm;
	ivec3 terrainDims = Volume->GetTerrainTotalDims();
	
	// we need the terrain block dims to be a positive power of 2 and have equal dimensions
	assert(terrainDims.x > 0);
	assert(terrainDims.y > 0);
	assert(terrainDims.z > 0);
	assert(terrainDims.x == terrainDims.y);
	assert(terrainDims.x == terrainDims.z);
	assert(IsAPowerOfTwo(terrainDims.x));
	assert(IsAPowerOfTwo(terrainDims.y));
	assert(IsAPowerOfTwo(terrainDims.z));

	ParentNode.MipLevel = GetMipLevel(terrainDims.x);
	PopulateDebugMipLevelColourTable(ParentNode.MipLevel);
	ParentNode.BottomLeftCorner = { 0,0,0 };
	ParentNode.SizeInVoxels = terrainDims.x;
	PopulateChildren(&ParentNode);
}

void TerrainOctree::GetChunksToRender(const glm::mat4& camMatrix, float aspect, float fovY, float zNear, float zFar, std::vector<TerrainOctreeNode*>& outNodesToRender)
{
	Frustum frustum;
	CameraFunctionLibrary::CreateFrustumFromCamera(camMatrix, aspect, fovY, zNear, zFar, frustum);
	glm::mat4 projection = glm::perspective(fovY, aspect, zNear, zFar);
	glm::mat4 viewMatrix = glm::inverse(camMatrix);
	glm::mat4 viewProjectionMatrix = projection * viewMatrix;
	GetChunksToRender(frustum, outNodesToRender, &ParentNode, viewProjectionMatrix);

}

void TerrainOctree::DebugVisualiseChunks(const std::vector<TerrainOctreeNode*>& nodes)
{
	glm::vec3 parentCenter = {
			ParentNode.BottomLeftCorner.x + (ParentNode.SizeInVoxels / 2),
			ParentNode.BottomLeftCorner.y + (ParentNode.SizeInVoxels / 2),
			ParentNode.BottomLeftCorner.z + (ParentNode.SizeInVoxels / 2)
	};
	Gizmos::AddBox(
		parentCenter,
		{ ParentNode.SizeInVoxels, ParentNode.SizeInVoxels, ParentNode.SizeInVoxels }, // dimensions
		false,
		{ 1.0,1.0,1.0,1.0 }
	);
	for (TerrainOctreeNode* node : nodes)
	{
		glm::vec3 center = { 
			node->BottomLeftCorner.x + (node->SizeInVoxels / 2),
			node->BottomLeftCorner.y + (node->SizeInVoxels / 2),
			node->BottomLeftCorner.z + (node->SizeInVoxels / 2)
		};
		
		Gizmos::AddBox(
			center,
			{node->SizeInVoxels, node->SizeInVoxels, node->SizeInVoxels}, // dimensions
			true,
			DebugMipLevelColourTable[node->MipLevel]
		);
	}
}

void TerrainOctree::GetChunksToRender(const Frustum& frustum, std::vector<TerrainOctreeNode*>& outNodesToRender, TerrainOctreeNode* onNode, const glm::mat4& viewProjectionMatrix)
{
	/*
	https://transvoxel.org/Lengyel-VoxelTerrain.pdf?fbclid=IwAR0xXDW0I_amFS0Tfp-YtyE9oB_va_SDVP4zdpb6D4Z8Lmb1Gvf-6212_EI
	The octree is traversed when the terrain is rendered, and any block not
	intersecting the view frustum is culled along with its entire subtree. When a block is visited in the
	octree and determined to be visible to the camera, its projected size in the viewport is calculated.
	If the size falls below a threshold value, then that block is rendered, and its subtree is skipped.
	
	Jims note: presumably it also renders chunks when they are in the viewport and the lowest mip level
	has been reached without the size threshold being satisfied, because if you're looking at the floor in a blck
	then you want that floor to be rendered in the highest detail
	*/
	for (int i = 0; i < 8; i++)
	{
		if (TerrainOctreeNode* child = onNode->Children[i].get())
		{
			glm::vec3 sphereCenter =
			{
				child->BottomLeftCorner.x + (float)child->SizeInVoxels / 2.0f,
				child->BottomLeftCorner.y + (float)child->SizeInVoxels / 2.0f,
				child->BottomLeftCorner.y + (float)child->SizeInVoxels / 2.0f
			};
			float sphereRadius = abs(glm::length(sphereCenter - glm::vec3(child->BottomLeftCorner))); // maybe abs not needed - todo - find out
			if (CameraFunctionLibrary::IsSphereInFrustum(sphereCenter, sphereRadius, frustum))
			{
				// here we need to determine the blocks projected size in the viewport and if it is 
				// below a threshold or, I think the lowst mip level, then add it to the output list 
				if (ViewportAreaHeuristic(child, viewProjectionMatrix) < MinimumViewportAreaThreshold || child->SizeInVoxels == 16)
				{
					outNodesToRender.push_back(child);
				}
				else 
				{
					GetChunksToRender(frustum, outNodesToRender, child, viewProjectionMatrix);
				}
			}
		}
	}
}

float TerrainOctree::ViewportAreaHeuristic(TerrainOctreeNode* block, const glm::mat4& viewProjectionMatrix)
{
	using namespace glm;
	vec4 corners[8];
	for (int x = 0; x < 2; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int z = 0; z < 2; z++)
			{
				vec3 corner = {
					block->BottomLeftCorner.x + x * block->SizeInVoxels,
					block->BottomLeftCorner.y + y * block->SizeInVoxels,
					block->BottomLeftCorner.y + z * block->SizeInVoxels
				};
				corners[z + 2 * y + 4 * x] = vec4(corner, 1.0f);
			}
		}
	}
	float minX = 100000.0f; // an arbitrary high number that any clipped coordinate will be <
	float maxX = -100000.0f;// an arbitrary low number that any clipped coordinate will be >
	float minY = 100000.0f;
	float maxY = -100000.0f;
	
	for (int i = 0; i < 8; i++)
	{
		// transform into clip space
		glm::vec4 clipSpace = viewProjectionMatrix * corners[i];

		// clip vertices to screen edge
		glm::vec2 clipped =
		{
			glm::clamp(clipSpace.x, -1.0f, 1.0f),
			glm::clamp(clipSpace.y, -1.0f, 1.0f)
		};

		// get bounding extents of clipped coords
		if (clipped.x < minX)
		{
			minX = clipped.x;
		}
		else if (clipped.x > maxX)
		{
			maxX = clipped.x;
		}

		if (clipped.y < minY)
		{
			minY = clipped.y;
		}
		else if (clipped.y > maxY)
		{
			maxY = clipped.y;
		}
	}

	// return area of extents
	float width = maxX - minX;
	float height = maxY - minY;
	return width * height;
}

void TerrainOctree::PopulateChildren(TerrainOctreeNode* node)
{
	i32 childDims = node->SizeInVoxels / 2;
	i32 childMipLevel = node->MipLevel - 1;

	for (i32 x = 0; x < 2; x++)
	{
		for (i32 y = 0; y < 2; y++)
		{
			for (i32 z = 0; z < 2; z++)
			{
				std::unique_ptr<TerrainOctreeNode>& child = node->Children[z + 2 * y + 4 * x];
				if (child.get()) 
				{
					child.reset();
				}
				child = std::make_unique<TerrainOctreeNode>();
				child->SizeInVoxels = childDims;
				child->MipLevel = childMipLevel;
				child->BottomLeftCorner = { x * childDims, y * childDims, z * childDims };
			}
		}
	}
	
	if (childDims > 16)
	{
		for (i32 i = 0; i < 8; i++)
		{
			PopulateChildren(node->Children[i].get());
		}
	}
	else
	{
		assert(childMipLevel == 0);
	}
	
}

bool TerrainOctree::IsAPowerOfTwo(i32 number)
{
	// https://www.educative.io/answers/how-to-check-if-a-number-is-a-power-of-2-in-cpp#:~:text=Approach%203-,To%20check%20if%20a%20given%20number%20is%20a%20power%20of,Otherwise%2C%20it%20is%20not.
	// calculate log2() of n
	float i = log2((float)number);

	// check if n is a power of 2
	if (ceil(i) == floor(i)) {
		return true;
	}
	else {
		return false;
	}
}

u32 TerrainOctree::GetMipLevel(i32 cellVoxelsWidth)
{

	u32 mipLevel = 0;
	while (cellVoxelsWidth != 16)
	{
		mipLevel++;
		cellVoxelsWidth /= 2;
	}
	return mipLevel;
}

void TerrainOctree::PopulateDebugMipLevelColourTable(u32 maximumMipLevel)
{
	static const glm::vec4 red = glm::vec4{ 1.0, 0.0, 0.0, 1.0};
	static const glm::vec4 green = glm::vec4{ 0.0, 1.0, 0.0, 1.0 };
	DebugMipLevelColourTable = std::make_unique<glm::vec4[]>(maximumMipLevel + 1);
	for (int i = 0; i <= maximumMipLevel; i++)
	{
		DebugMipLevelColourTable[i] = glm::mix(red, green, (float)i / (float)maximumMipLevel);
	}
}
