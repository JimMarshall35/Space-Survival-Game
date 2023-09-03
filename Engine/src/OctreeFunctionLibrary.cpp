#include "OctreeFunctionLibrary.h"
#include "OctreeTypes.h"
#include "Camera.h"
#include "TerrainDefs.h"

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
