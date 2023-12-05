#pragma once
#include "CommonTypedefs.h"
#include "Core.h"

// a stack of visited children to get to a node - each nibble is an index into children
// to be used for serialization.
typedef u64 TerrainOctreeIndex;

enum class TerrainChunkMeshBuffer
{
	VBO = 0,
	EBO = 1
};

struct APP_API TerrainChunkTransitionMesh
{
	u32 VAO;
	u32 Buffers[2];
	u32 IndiciesToDraw;
};

struct APP_API TerrainChunkMesh
{
	u32 VAO;
	u32 Buffers[2];
	u32 IndiciesToDraw;
	TerrainChunkTransitionMesh TransitionMeshes[6];
	bool bNeedsRegenerating;
	inline u32 GetVBO() const { return Buffers[(u32)TerrainChunkMeshBuffer::VBO]; }
	inline u32 GetEBO() const { return Buffers[(u32)TerrainChunkMeshBuffer::EBO]; }

};