#pragma once
#include <glm.hpp>
using namespace glm;
class ITerrainVoxelVolume
{
public:
	virtual ivec3 GetTerrainTotalDims() = 0;
};