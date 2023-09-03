#pragma once
#include "CommonTypedefs.h"

#include <glm.hpp>

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
