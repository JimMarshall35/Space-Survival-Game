#pragma once
#include <unordered_set>
#include "OctreeTypes.h"


class IVoxelDataSource;

namespace OctreeSerialisation
{
	void SaveNewlyGeneratedToFile(const std::unordered_set<TerrainOctreeIndex>& setNodex, IVoxelDataSource* voxelDataSource, const char* path);
	void LoadFromFile(IVoxelDataSource* voxelDataSource, const char* path);
}