#include "SparseTerrainVoxelOctree.h"
#include <ext.hpp>
#include <cassert>
#include "IAllocator.h"
#include "OctreeFunctionLibrary.h"
#include "CameraFunctionLibrary.h"
#include "Camera.h"
#include "TerrainLODSelectionAndCullingAlgorithm.h"
#include "TerrainDefs.h"
#include "ITerrainPolygonizer.h"
#include "ITerrainGraphicsAPIAdaptor.h"
#include <future>

SparseTerrainVoxelOctree::SparseTerrainVoxelOctree(IAllocator* allocator, ITerrainPolygonizer* polygonizer, ITerrainGraphicsAPIAdaptor* graphicsAPIAdaptor, u32 sizeVoxels, i8 clampValueHigh, i8 clampValueLow)
	:Allocator(allocator),
	Polygonizer(polygonizer),
	GraphicsAPIAdaptor(graphicsAPIAdaptor),
	ParentNode(OctreeFunctionLibrary::GetMipLevel(sizeVoxels), { 0,0,0 }, sizeVoxels),
	ParentNodeStack(IAllocator::NewArray<SparseTerrainOctreeNode*>(Allocator, ParentNode.MipLevel + 1)),
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

TerrainOctreeIndex SparseTerrainVoxelOctree::SetVoxelAt_Internal(const glm::ivec3& location, i8 value)
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

void SparseTerrainVoxelOctree::GetVoxelsForNode(ITerrainOctreeNode* node, i8* outVoxels)
{
	u32 sizeInVoxels = node->GetSizeInVoxels();
	const glm::ivec3& bottomLeft = node->GetBottomLeftCorner();
	i32 stepSize = sizeInVoxels / 16;
	i32 x = bottomLeft.x - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 y = bottomLeft.y - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 z = bottomLeft.z - POLYGONIZER_NEGATIVE_GUTTER * stepSize;
	i32 endX = bottomLeft.x + sizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;
	i32 endY = bottomLeft.y + sizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;
	i32 endZ = bottomLeft.z + sizeInVoxels + POLYGONIZER_POSITIVE_GUTTER * stepSize;

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

void SparseTerrainVoxelOctree::SetVoxelAt(const glm::ivec3& location, i8 value)
{
	TerrainOctreeIndex index = SetVoxelAt_Internal(location, value);
	//
}

void SparseTerrainVoxelOctree::Clear()
{
	DeleteAllChildren(&ParentNode);
}

void SparseTerrainVoxelOctree::ResizeAndClear(const size_t newSize)
{
	DeleteAllChildren(&ParentNode);
	ParentNode.SizeInVoxels = newSize;
	ParentNode.MipLevel = OctreeFunctionLibrary::GetMipLevel(newSize);
	Allocator->Free(ParentNodeStack);
	IAllocator::NewArray<SparseTerrainOctreeNode*>(Allocator, ParentNode.MipLevel + 1);
}

size_t SparseTerrainVoxelOctree::GetSize() const
{
	return ParentNode.SizeInVoxels;
}

SparseTerrainVoxelOctree::SparseTerrainOctreeNode* SparseTerrainVoxelOctree::FindNodeFromIndex(TerrainOctreeIndex index)
{
	SparseTerrainOctreeNode* onNode = &ParentNode;
	for (u32 i = 0; i < ParentNode.MipLevel; i++)
	{
		u32 thisIndex = (index >> (4 * i)) & 0x0f;
		onNode = onNode->Children[thisIndex];
		assert(onNode);
	}
	return onNode;
}


void SparseTerrainVoxelOctree::GetChunksToRender(const Camera& camera, float aspect, float fovY, float zNear, float zFar, std::vector<ITerrainOctreeNode*>& outNodesToRender)
{
	Frustum frustum = CameraFunctionLibrary::CreateFrustumFromCamera(camera, aspect, fovY, zNear, zFar);
	glm::mat4 projection = glm::perspective(fovY, aspect, zNear, zFar);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	glm::mat4 viewProjectionMatrix = projection * viewMatrix;

	std::vector<std::future<PolygonizeWorkerThreadData*>> polygonizedNodeFutures;
	
	TerrainLODSelectionAndCullingAlgorithm::GetChunksToRender(frustum, outNodesToRender, &ParentNode, viewProjectionMatrix,
	[&polygonizedNodeFutures, this](ITerrainOctreeNode* node) {
		// every time the terrain chunk selection algorithm pushes a chunk to render that needs to be polygonized,
		// queue an async operation to polygonize it 
		polygonizedNodeFutures.push_back(Polygonizer->PolygonizeNodeAsync(node, this));
	});

	for (auto& future : polygonizedNodeFutures)
	{
		// upload the newly generated polygon data to a GPU buffer (fill in TerrainChunkMesh) and free the raw vertex data
		PolygonizeWorkerThreadData* data = future.get();
		GraphicsAPIAdaptor->UploadNewlyPolygonizedToGPU(data);
		data->MyAllocator->Free(data->GetPtrToDeallocate());
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

const TerrainChunkMesh& SparseTerrainVoxelOctree::SparseTerrainOctreeNode::GetTerrainChunkMesh() const
{
	return Mesh;
}

void SparseTerrainVoxelOctree::SparseTerrainOctreeNode::SetTerrainChunkMesh(const TerrainChunkMesh& mesh)
{
	Mesh = mesh;
}
