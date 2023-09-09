#include "TerrainLODSelectionAndCullingAlgorithm.h"
#include "CameraFunctionLibrary.h"
#include "ITerrainOctreeNode.h"

float TerrainLODSelectionAndCullingAlgorithm::MinimumViewportAreaThreshold = 1.0;

void TerrainLODSelectionAndCullingAlgorithm::GetChunksToRender(const Frustum& frustum, std::vector<ITerrainOctreeNode*>& outNodesToRender, ITerrainOctreeNode* onNode, const glm::mat4& viewProjectionMatrix, std::function<void(ITerrainOctreeNode*) > needsRegeneratingCallback)
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
		ITerrainOctreeNode* child = onNode->GetChild(i);
		if (child != nullptr)
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
					if (needsRegeneratingCallback && child->NeedsRegenerating())
					{
						needsRegeneratingCallback(child);
					}
				}
				else
				{
					GetChunksToRender(frustum, outNodesToRender, child, viewProjectionMatrix, needsRegeneratingCallback);
				}
			}
		}
		else if (onNode->GetMipLevel() == 0)
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
		// TODO: we should maybe clamp clipsace coords to the size of the screen here

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
