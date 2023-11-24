#pragma once
#include "Shader.h"
#include "ITerrainGraphicsAPIAdaptor.h"
#include "CommonTypedefs.h"
#include <unordered_map>
#include <chrono>
#include <variant>
#include <glm.hpp>

class ITerrainOctreeNode;

typedef std::chrono::time_point<std::chrono::system_clock> TerrainTimePoint;
typedef std::chrono::duration<u32> TerrainDuration;

struct TerrainNodeRendererData
{
	TerrainTimePoint LastRendered;
	u32 SizeOfAllocation;
};





typedef std::pair<ITerrainOctreeNode*, TerrainNodeRendererData> LastRenderedPair;

typedef std::unordered_map<ITerrainOctreeNode*, TerrainNodeRendererData> LastRenderedMap;

enum class GPUMemoryFreeingStrategy
{
	OvershootFixedAmount,
	FreeUntilLastRenderedAFixedTimeAgo
};

struct TerrainRendererConfigData
{
	size_t MemoryBudget;
	GPUMemoryFreeingStrategy FreeingStrategy;
	std::variant<u32, TerrainDuration> FreeingData;
};

class TerrainRenderer : public ITerrainGraphicsAPIAdaptor
{
public:
	TerrainRenderer(){};
	TerrainRenderer(const TerrainRendererConfigData& memoryBudget);

	// Inherited via ITerrainGraphicsAPIAdaptor
	virtual void UploadNewlyPolygonizedToGPU(PolygonizeWorkerThreadData* data) override;

	// Inherited via ITerrainGraphicsAPIAdaptor
	virtual void RenderTerrainNodes(
		const std::vector<ITerrainOctreeNode*>& nodes,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, bool renderDebugBoxes = false) override;

	// Inherited via ITerrainGraphicsAPIAdaptor
	virtual void SetTerrainMaterial(const TerrainMaterial& material) override;

	virtual void SetTerrainLight(const TerrainLight& light) override;

private:

	void FreeChunksToFit(u32 attemptedAllocation);

	void FreeChunksToFit_UntilFixedMemoryAmount(u32 attemptedAllocation);

	void FreeChunksToFit_UntilFixedTimeAgoRendered(u32 attemptedAllocation);

private:
	size_t CurrentTerrainGPUAllocation = 0;
	
	size_t MemoryBudget;

	Shader TerrainShader;

	LastRenderedMap LastRendered;

	GPUMemoryFreeingStrategy FreeingStrategy;

	std::variant<u32, TerrainDuration> FreeingData;
};