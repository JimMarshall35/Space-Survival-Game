#include "TerrainOctree.h"
#include "ITerrainVoxelVolume.h"
#include <cassert>
#include <cmath>
#include "CameraFunctionLibrary.h"
#include <ext.hpp>
#define BASE_CELL_SIZE 16 // the size of all cells. lowest mip level is sampled 16x16x16 in actual dimensions with a cell size of 2, next level is 32x32x32 ect with a cell size of 4, ect. 
#define POLYGONIZER_NEGATIVE_GUTTER 1
#define POLYGONIZER_POSITIVE_GUTTER 2
#include "Gizmos.h"
#include "IAllocator.h"


DebugVisualizerTerrainOctree::DebugVisualizerTerrainOctree(ITerrainVoxelVolume* volume)
	:Volume(volume),
	LODCulling(1.0)
{
	MakeQuadtree();
}

void DebugVisualizerTerrainOctree::MakeQuadtree()
{
	using namespace glm;
	ivec3 terrainDims = Volume->GetTerrainTotalDims();
	
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
	LODCulling.GetChunksToRender(frustum, outNodesToRender, &ParentNode, viewProjectionMatrix);
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

bool OctreeFunctionLibrary::IsAPowerOfTwo(i32 number)
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

u32 OctreeFunctionLibrary::GetMipLevel(i32 cellVoxelsWidth)
{
	assert(IsAPowerOfTwo(cellVoxelsWidth));
	u32 mipLevel = 0;
	while (cellVoxelsWidth != BASE_CELL_SIZE)
	{
		mipLevel++;
		cellVoxelsWidth /= 2;
	}
	return mipLevel;
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

SparseTerrainVoxelOctree::SparseTerrainVoxelOctree(IAllocator* allocator, u32 sizeInVoxels, i8 clampValueHigh, i8 clampValueLow)
	:Allocator(allocator),
	ParentNode(OctreeFunctionLibrary::GetMipLevel(sizeInVoxels), { 0,0,0 }, sizeInVoxels),
	ParentNodeStack(IAllocator::NewArray<SparseTerrainOctreeNode*>(Allocator, ParentNode.MipLevel+1)),
	VoxelClampValueHigh(clampValueHigh),
	VoxelClampValueLow(clampValueLow),
	VoxelDefaultValue(clampValueHigh)
{
	

}

SparseTerrainVoxelOctree::~SparseTerrainVoxelOctree()
{
	DeleteAllChildren(&ParentNode);
	Allocator->Free(ParentNodeStack);
}

TerrainOctreeIndex SparseTerrainVoxelOctree::SetVoxelAt(const glm::ivec3& location, i8 value)
{

	if (value > VoxelClampValueHigh || value < VoxelClampValueLow)
	{
		return 0xffffffffffffffff;
	}

	SparseTerrainOctreeNode* onNode = &ParentNode;
	size_t ParentNodeStackPtr = 0;
	TerrainOctreeIndex outIndex = 0;

	u32 shiftCounter = 0;

	while (onNode->MipLevel != 0)
	{
		ParentNodeStack[ParentNodeStackPtr++] = onNode;
		u8 childIndex = 0xff;
		onNode = FindChildContainingPoint(onNode, location, childIndex);
		assert(onNode);
		outIndex |= (TerrainOctreeIndex)(childIndex << (4 * (shiftCounter++)));
	}
	if (!onNode->VoxelData)
	{
		static const size_t voxelDataAllocationSize = BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE;
		onNode->VoxelData = IAllocator::NewArray<i8>(Allocator, voxelDataAllocationSize);
		memset(onNode->VoxelData, VoxelDefaultValue, voxelDataAllocationSize);
	}
	glm::ivec3 voxelDataLocation = GetLocationWithinMipZeroCellFromWorldLocation(onNode, location);

	size_t voxelDataIndex = voxelDataLocation.x + BASE_CELL_SIZE * voxelDataLocation.y + BASE_CELL_SIZE * BASE_CELL_SIZE * voxelDataLocation.y;
	if (onNode->VoxelData[voxelDataIndex] != value)
	{
		onNode->VoxelData[voxelDataIndex] = value;

		for (i32 i = 0; i < ParentNodeStackPtr; i++)
		{
			ParentNodeStack[i]->Mesh.bNeedsRegenerating = true;
		}

		onNode->Mesh.bNeedsRegenerating = true;
	}
	
	
	return outIndex;
}

i8 SparseTerrainVoxelOctree::GetVoxelAt(const glm::ivec3& location)
{
	u8 outIndex;
	SparseTerrainOctreeNode* onNode = &ParentNode;
	if (!OctreeFunctionLibrary::IsPointInCube(location, onNode->BottomLeftCorner, onNode->SizeInVoxels))
	{
		return VoxelDefaultValue;
	}
	while (onNode->MipLevel != 0)
	{
		assert(OctreeFunctionLibrary::IsPointInCube(location, onNode->BottomLeftCorner, onNode->SizeInVoxels));
		i32 childDims = onNode->SizeInVoxels / 2;
		i32 childMipLevel = onNode->MipLevel - 1;

		// find which child the point is in and set onNode to that child
		if (auto child = FindChildContainingPoint(onNode, location, outIndex, false))
		{
			onNode = child;
		}
		else
		{
			return VoxelDefaultValue;
		}
	}
	assert(onNode->VoxelData);
	glm::ivec3 voxelDataLocation = GetLocationWithinMipZeroCellFromWorldLocation(onNode, location);
	size_t voxelDataIndex = voxelDataLocation.x + BASE_CELL_SIZE * voxelDataLocation.y + BASE_CELL_SIZE * BASE_CELL_SIZE * voxelDataLocation.y;
	return onNode->VoxelData[voxelDataIndex];
}

SparseTerrainVoxelOctree::SparseTerrainOctreeNode* SparseTerrainVoxelOctree::FindNodeFromIndex(TerrainOctreeIndex index)
{
	SparseTerrainOctreeNode* onNode = &ParentNode;
	for (u32 i = 0; i < ParentNode.MipLevel; i++)
	{
		u32 thisIndex = (index >> (4*i)) & 0x0f;
		onNode = onNode->Children[thisIndex];
		assert(onNode);
	}
	return onNode;
}

void SparseTerrainVoxelOctree::FillPrePolygonizingVoxelStagingArea(SparseTerrainOctreeNode* node, i8* outVoxels)
{

	i32 stepSize = node->SizeInVoxels / 16;
	i32 x = node->BottomLeftCorner.x - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 y = node->BottomLeftCorner.y - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 z = node->BottomLeftCorner.z - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 endX = node->BottomLeftCorner.x + node->SizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;
	i32 endY = node->BottomLeftCorner.y + node->SizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;
	i32 endZ = node->BottomLeftCorner.z + node->SizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;

	for (x; x < endX; x += stepSize)
	{
		for (y; y < endY; y += stepSize)
		{
			for (z; z < endZ; z += stepSize)
			{
				*(outVoxels++) = GetVoxelAt(glm::ivec3{ x,y,z });
			}
		}
	}
}

SparseTerrainVoxelOctree::SparseTerrainOctreeNode* SparseTerrainVoxelOctree::FindChildContainingPoint(SparseTerrainOctreeNode* onNode, const glm::ivec3& location, u8& outChildIndex, bool allocateNewIfNull)
{
	assert(OctreeFunctionLibrary::IsPointInCube(location, onNode->BottomLeftCorner, onNode->SizeInVoxels));
	i32 childDims = onNode->SizeInVoxels / 2;
	i32 childMipLevel = onNode->MipLevel - 1;
	for (i32 x = 0; x < 2; x++)
	{
		for (i32 y = 0; y < 2; y++)
		{
			for (i32 z = 0; z < 2; z++)
			{
				i32 i = z + (2 * y) + (4 * x);

				glm::ivec3& childBL =
				glm::ivec3{
					onNode->BottomLeftCorner.x + x * childDims,
					onNode->BottomLeftCorner.y + y * childDims,
					onNode->BottomLeftCorner.z + z * childDims
				};
				if (OctreeFunctionLibrary::IsPointInCube(location, childBL, childDims))
				{
					if (!onNode->Children[i] && allocateNewIfNull)
					{
						onNode->Children[i] = IAllocator::New<SparseTerrainOctreeNode>(Allocator);
						*onNode->Children[i] = SparseTerrainOctreeNode(childMipLevel, childBL, childDims);
					}
					outChildIndex = i;
					return onNode->Children[i];
				}
			}
		}
	}
	assert(false); // should never get here
	return nullptr;
}

void SparseTerrainVoxelOctree::DeleteAllChildren(SparseTerrainOctreeNode* node)
{
	for (i32 i = 0; i < 8; i++)
	{
		if (node->Children[i])
		{
			DeleteAllChildren(node->Children[i]);
		}
	}
	if (node != &ParentNode)
	{
		if (node->VoxelData)
		{
			Allocator->Free(node->VoxelData);
		}
		Allocator->Free(node);
	}
}

glm::ivec3 SparseTerrainVoxelOctree::GetLocationWithinMipZeroCellFromWorldLocation(SparseTerrainOctreeNode* mipZeroCell, const glm::ivec3& globalLocation)
{
	assert(mipZeroCell->MipLevel == 0);
	glm::ivec3 voxelDataLocation = globalLocation - mipZeroCell->BottomLeftCorner;
	assert(voxelDataLocation.x < BASE_CELL_SIZE);
	assert(voxelDataLocation.y < BASE_CELL_SIZE);
	assert(voxelDataLocation.z < BASE_CELL_SIZE);
	return voxelDataLocation;
}



TerrainLODSelectionAndCullingAlgorithm::TerrainLODSelectionAndCullingAlgorithm(float minimumViewportAreaThreshold)
	:MinimumViewportAreaThreshold(minimumViewportAreaThreshold)
{
}

void TerrainLODSelectionAndCullingAlgorithm::GetChunksToRender(const Frustum& frustum, std::vector<ITerrainOctreeNode*>& outNodesToRender, ITerrainOctreeNode* onNode, const glm::mat4& viewProjectionMatrix)
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
		if (ITerrainOctreeNode* child = onNode->GetChild(i))
		{
			const glm::ivec3& bottomLeft = child->GetBottomLeftCorner();
			u32 sizeInVoxels = child->GetSizeInVoxels();
			glm::vec3 sphereCenter =
			{
				bottomLeft.x + (float)sizeInVoxels / 2.0f,
				bottomLeft.y + (float)sizeInVoxels / 2.0f,
				bottomLeft.z + (float)sizeInVoxels / 2.0f
			};
			float sphereRadius = abs(glm::length(sphereCenter - glm::vec3(bottomLeft))); // maybe abs not needed - todo - find out
			if (CameraFunctionLibrary::IsSphereInFrustum(sphereCenter, sphereRadius, frustum))
			{
				// here we need to determine the blocks projected size in the viewport and if it is 
				// below a threshold or, I think the lowst mip level, then add it to the output list 
				float val = ViewportAreaHeuristic(child, viewProjectionMatrix);
				if (val < MinimumViewportAreaThreshold && val > 0.0f)
				{
					outNodesToRender.push_back(child);
				}
				else
				{
					GetChunksToRender(frustum, outNodesToRender, child, viewProjectionMatrix);
				}
			}
		}
		else if(onNode->GetMipLevel() == 0)
		{
			outNodesToRender.push_back(onNode);
		}
	}
}

float TerrainLODSelectionAndCullingAlgorithm::ViewportAreaHeuristic(ITerrainOctreeNode* block, const glm::mat4& viewProjectionMatrix)
{
	using namespace glm;
	vec4 corners[8];
	for (int x = 0; x < 2; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int z = 0; z < 2; z++)
			{
				const glm::ivec3& bottomLeft = block->GetBottomLeftCorner();
				u32 sizeInVoxels = block->GetSizeInVoxels();
				vec3 corner = {
					bottomLeft.x + x * sizeInVoxels,
					bottomLeft.y + y * sizeInVoxels,
					bottomLeft.z + z * sizeInVoxels
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
		clipSpace = {
			clipSpace.x / clipSpace.w,
			clipSpace.y / clipSpace.w,
			clipSpace.z / clipSpace.w,
			clipSpace.w
		};

		// get bounding extents of clipped coords
		if (clipSpace.x < minX)
		{
			minX = clipSpace.x;
		}
		else if (clipSpace.x > maxX)
		{
			maxX = clipSpace.x;
		}

		if (clipSpace.y < minY)
		{
			minY = clipSpace.y;
		}
		else if (clipSpace.y > maxY)
		{
			maxY = clipSpace.y;
		}
	}

	// return area of extents
	float width = maxX - minX;
	float height = maxY - minY;
	return width * height;
}

u32 OctreeFunctionLibrary::GetSizeInVoxels(u32 mipLevel)
{
	u32 size = BASE_CELL_SIZE;
	for (u32 i = 0; i < mipLevel; i++)
	{
		size *= 2;
	}
	return size;
}

bool OctreeFunctionLibrary::IsPointInCube(const glm::ivec3& point, const glm::ivec3& cubeBottomLeft, u32 cubeSizeVoxels)
{
	return point.x >= cubeBottomLeft.x && point.x < cubeBottomLeft.x + cubeSizeVoxels
		&& point.y >= cubeBottomLeft.y && point.y < cubeBottomLeft.y + cubeSizeVoxels
		&& point.z >= cubeBottomLeft.z && point.z < cubeBottomLeft.z + cubeSizeVoxels;
}

SparseTerrainVoxelOctree::SparseTerrainOctreeNode::SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner, u32 sizeInVoxels)
	:MipLevel(mipLevel),
	BottomLeftCorner(bottomLeftCorner),
	SizeInVoxels(sizeInVoxels)
{
}

SparseTerrainVoxelOctree::SparseTerrainOctreeNode::SparseTerrainOctreeNode(u32 mipLevel, const ivec3& bottomLeftCorner)
	:MipLevel(mipLevel),
	BottomLeftCorner(bottomLeftCorner),
	SizeInVoxels(OctreeFunctionLibrary::GetSizeInVoxels(mipLevel))
{
}
