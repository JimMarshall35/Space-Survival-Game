#include <glm.hpp>
#include "CommonTypedefs.h"
struct ITerrainOctreeNode;

class IVoxelDataSource
{
public:
	// need to make both getters const but can't due to FindChildContainingPoint implementation
	// TODO: sort out
	virtual i8 GetVoxelAt(const glm::ivec3& valueAt) = 0;
	virtual void GetVoxelsForNode(ITerrainOctreeNode* node, i8* outVoxels) = 0;
	virtual void SetVoxelAt(const glm::ivec3& location, i8 value) = 0;
};