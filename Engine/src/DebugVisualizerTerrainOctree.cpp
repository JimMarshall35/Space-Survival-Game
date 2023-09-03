#include "DebugVisualizerTerrainOctree.h"
#include <cassert>
#include <cmath>
#include "CameraFunctionLibrary.h"

#include "Gizmos.h"
#include "IAllocator.h"
#include "OctreeFunctionLibrary.h"
#include "TerrainLODSelectionAndCullingAlgorithm.h"
#include "TerrainDefs.h"



DebugVisualizerTerrainOctree::DebugVisualizerTerrainOctree()
{
	TerrainLODSelectionAndCullingAlgorithm::MinimumViewportAreaThreshold = 1.0f;
	MakeQuadtree();
}

void DebugVisualizerTerrainOctree::MakeQuadtree()
{
	using namespace glm;
	ivec3 terrainDims = ivec3{ 2048, 2048, 2048 };
	
	// we need the terrain block dims to be a positive power of 2 and have equal dimensions
	assert(terrainDims.x > 0);
	assert(terrainDims.y > 0);
	assert(terrainDims.z > 0);
	assert(terrainDims.x == terrainDims.y);
	assert(terrainDims.x == terrainDims.z);
	assert(OctreeFunctionLibrary::IsAPowerOfTwo(terrainDims.x));
	assert(OctreeFunctionLibrary::IsAPowerOfTwo(terrainDims.y));
	assert(OctreeFunctionLibrary::IsAPowerOfTwo(terrainDims.z));

	ParentNode.MipLevel = OctreeFunctionLibrary::GetMipLevel(terrainDims.x);
	DebugMipLevelToDraw = ParentNode.MipLevel;
	PopulateDebugMipLevelColourTable(ParentNode.MipLevel);
	ParentNode.BottomLeftCorner = { 0,0,0 };
	ParentNode.SizeInVoxels = terrainDims.x;
	PopulateChildren(&ParentNode);
}

void DebugVisualizerTerrainOctree::GetChunksToRender(const Camera& cam, float aspect, float fovY, float zNear, float zFar, std::vector<ITerrainOctreeNode*>& outNodesToRender)
{
	Frustum frustum = CameraFunctionLibrary::CreateFrustumFromCamera(cam, aspect, fovY, zNear, zFar);
	glm::mat4 projection = glm::perspective(fovY, aspect, zNear, zFar);
	glm::mat4 viewMatrix = cam.GetViewMatrix();
	glm::mat4 viewProjectionMatrix = projection * viewMatrix;
	TerrainLODSelectionAndCullingAlgorithm::GetChunksToRender(frustum, outNodesToRender, &ParentNode, viewProjectionMatrix);
	DebugCamera = cam;
	bDebugCameraSet = true;
}

void DebugVisualizerTerrainOctree::DebugVisualiseChunks(const std::vector<ITerrainOctreeNode*>& nodes)
{
	if (DebugMipLevelToDraw > -1)
	{
		DebugVisualiseMipLevel(&ParentNode, DebugMipLevelToDraw, { 0,0,0,1 });
	}
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
	for (ITerrainOctreeNode* node : nodes)
	{
		const glm::ivec3& bottomLeft = node->GetBottomLeftCorner();
		u32 sizeInVoxels = node->GetSizeInVoxels();

		glm::vec3 center = { 
			bottomLeft.x + (sizeInVoxels / 2),
			bottomLeft.y + (sizeInVoxels / 2),
			bottomLeft.z + (sizeInVoxels / 2)
		};
		
		Gizmos::AddBox(
			center,
			{ sizeInVoxels, sizeInVoxels, sizeInVoxels }, // dimensions
			bDebugFill,
			DebugMipLevelColourTable[node->GetMipLevel()]
		);
	}

	if (bDebugCameraSet)
	{
		glm::vec3 forward = DebugCamera.Front;
		glm::vec3 right = DebugCamera.Right;
		glm::vec3 up = DebugCamera.Up;
		glm::vec3 translation = DebugCamera.Position;
		//Gizmos::AddSphere(translation, 10, 10, 0.5, { 1.0,1.0,0.0,1.0 });

		Gizmos::AddLine(translation, translation + glm::normalize(forward) * 1.0f, { 0.0,0.0,1.0,1.0 });
		Gizmos::AddLine(translation, translation + glm::normalize(up) * 1.0f, { 0.0,1.0,0.0,1.0 });
		Gizmos::AddLine(translation, translation + glm::normalize(right) * 1.0f, { 1.0,0.0,0.0,1.0 });
	}
	
}

f32 DebugVisualizerTerrainOctree::GetMinimumViewportAreaThreshold()
{
	return TerrainLODSelectionAndCullingAlgorithm::MinimumViewportAreaThreshold;
}

void DebugVisualizerTerrainOctree::SetMinimumViewportAreaThreshold(f32 val)
{
	TerrainLODSelectionAndCullingAlgorithm::MinimumViewportAreaThreshold = val;
}



void DebugVisualizerTerrainOctree::PopulateChildren(TerrainOctreeNode* node)
{
	i32 childDims = node->SizeInVoxels / 2;
	i32 childMipLevel = node->MipLevel - 1;

	for (i32 x = 0; x < 2; x++)
	{
		for (i32 y = 0; y < 2; y++)
		{
			for (i32 z = 0; z < 2; z++)
			{
				i32 i = z + (2 * y) + (4 * x);
				std::unique_ptr<TerrainOctreeNode>& child = node->Children[i];
				if (child.get()) 
				{
					child.reset();
				}
				child = std::make_unique<TerrainOctreeNode>();
				child->SizeInVoxels = childDims;
				child->MipLevel = childMipLevel;
				glm::ivec3& parentBottomLeft = node->BottomLeftCorner;
				child->BottomLeftCorner = 
				{ 
					parentBottomLeft.x + x * childDims, 
					parentBottomLeft.y + y * childDims, 
					parentBottomLeft.z + z * childDims 
				};
			}
		}
	}
	
	if (childDims > BASE_CELL_SIZE)
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


void DebugVisualizerTerrainOctree::PopulateDebugMipLevelColourTable(u32 maximumMipLevel)
{
	static const glm::vec4 red = glm::vec4{ 1.0, 0.0, 0.0, 1.0};
	static const glm::vec4 green = glm::vec4{ 0.0, 1.0, 0.0, 1.0 };
	DebugMipLevelColourTable = std::make_unique<glm::vec4[]>(maximumMipLevel + 1);
	for (int i = 0; i <= maximumMipLevel; i++)
	{
		DebugMipLevelColourTable[i] = glm::mix(red, green, (float)i / (float)maximumMipLevel);
	}
}

void DebugVisualizerTerrainOctree::DebugVisualiseMipLevel(TerrainOctreeNode* node, u32 mipLevel, const glm::vec4& colour)
{
	if (node->MipLevel == mipLevel)
	{
		glm::vec3 center = {
			node->BottomLeftCorner.x + (node->SizeInVoxels / 2),
			node->BottomLeftCorner.y + (node->SizeInVoxels / 2),
			node->BottomLeftCorner.z + (node->SizeInVoxels / 2)
		};

		Gizmos::AddBox(
			center,
			{ node->SizeInVoxels, node->SizeInVoxels, node->SizeInVoxels }, // dimensions
			bDebugFill,
			DebugMipLevelColourTable[node->MipLevel]
		);
	}
	else
	{
		for (i32 i = 0; i < 8; i++)
		{
			if (TerrainOctreeNode* childNode = node->Children[i].get())
			{
				DebugVisualiseMipLevel(childNode, mipLevel, colour);
			}
		}
	}
}


ITerrainOctreeNode* DebugVisualizerTerrainOctree::TerrainOctreeNode::GetChild(u8 child) const
{
	return Children[child].get();
}

const ivec3& DebugVisualizerTerrainOctree::TerrainOctreeNode::GetBottomLeftCorner() const
{
	return BottomLeftCorner;
}

u32 DebugVisualizerTerrainOctree::TerrainOctreeNode::GetSizeInVoxels() const
{
	return SizeInVoxels;
}

u32 DebugVisualizerTerrainOctree::TerrainOctreeNode::GetMipLevel() const
{
	return MipLevel;
}

const TerrainChunkMesh& DebugVisualizerTerrainOctree::TerrainOctreeNode::GetTerrainChunkMesh() const
{
	return Mesh;
}

void DebugVisualizerTerrainOctree::TerrainOctreeNode::SetTerrainChunkMesh(const TerrainChunkMesh& mesh)
{
	Mesh = mesh;
}
