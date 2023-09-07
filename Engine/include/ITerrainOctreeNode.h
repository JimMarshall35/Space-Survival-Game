#pragma once
#include "CommonTypedefs.h"
#include <glm.hpp>
struct TerrainChunkMesh;

struct ITerrainOctreeNode
{
	virtual ITerrainOctreeNode* GetChild(u8 child) const = 0;
	virtual const glm::ivec3& GetBottomLeftCorner() const = 0;
	virtual u32 GetSizeInVoxels() const = 0;
	virtual u32 GetMipLevel() const = 0;
	virtual const TerrainChunkMesh& GetTerrainChunkMesh() const = 0;
	virtual TerrainChunkMesh& GetTerrainChunkMeshMutable() = 0;
	virtual void SetTerrainChunkMesh(const TerrainChunkMesh& mesh) = 0;
	virtual bool NeedsRegenerating() const = 0;
};