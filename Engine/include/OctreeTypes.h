#pragma once
#include "CommonTypedefs.h"

// a stack of visited children to get to a node - each nibble is an index into children
// to be used for serialization.
typedef u64 TerrainOctreeIndex;

struct TerrainChunkMesh
{
	u32 VAO = 0xffffffff;
	u32 VBO = 0xffffffff;
	bool bNeedsRegenerating;
};