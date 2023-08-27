#include "TerrainVoxelVolume.h"
#include <glm.hpp>
#include "Gizmos.h"
void TerrainVoxelVolume::DebugVisualiseVolume()
{
    float halfDimensionsInVoxels = DimensionsInVoxels/ 2;
    glm::vec3 center = { halfDimensionsInVoxels, halfDimensionsInVoxels, halfDimensionsInVoxels };
    glm::vec3 dims = { DimensionsInVoxels, halfDimensionsInVoxels, halfDimensionsInVoxels };
    Gizmos::AddBox(center, dims, false);
}

ivec3 TerrainVoxelVolume::GetTerrainTotalDims()
{
    return ivec3(DimensionsInVoxels, DimensionsInVoxels, DimensionsInVoxels);
}
