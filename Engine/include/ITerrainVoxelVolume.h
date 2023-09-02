#pragma once
#include <glm.hpp>
#include "Core.h"

using namespace glm;
class APP_API ITerrainVoxelVolume
{
public:
	virtual ivec3 GetTerrainTotalDims() = 0;
};