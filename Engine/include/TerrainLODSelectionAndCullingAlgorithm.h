#pragma once
#include <glm.hpp>
#include <vector>
#include <functional>

struct Frustum;
struct ITerrainOctreeNode;


class TerrainLODSelectionAndCullingAlgorithm
{
public:
	static void GetChunksToRender(
		const Frustum& frustum,
		std::vector<ITerrainOctreeNode*>& outNodesToRender,
		ITerrainOctreeNode* onNode,
		const glm::mat4& viewProjectionMatrix,
		std::function<void(ITerrainOctreeNode*)> needsRegeneratingCallback = std::function<void(ITerrainOctreeNode*)>{});

	// estimate how much space a block will take up in the view port
	static float ViewportAreaHeuristic(ITerrainOctreeNode* block, const glm::mat4& viewProjectionMatrix);

	// if the viewport area heuristic for a block is < this value then
	// the block will be rendered and it's subtree skipped.
	static float MinimumViewportAreaThreshold;
};