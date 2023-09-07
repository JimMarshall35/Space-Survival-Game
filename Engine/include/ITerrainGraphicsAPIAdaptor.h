#pragma once
#include <vector>
#include <glm.hpp>


struct PolygonizeWorkerThreadData;
struct ITerrainOctreeNode;
struct TerrainMaterial;
struct TerrainLight;

class ITerrainGraphicsAPIAdaptor
{
public:
	virtual void UploadNewlyPolygonizedToGPU(PolygonizeWorkerThreadData* data) = 0;
	virtual void RenderTerrainNodes(
		const std::vector<ITerrainOctreeNode*>& nodes,
		const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) = 0;
	virtual void SetTerrainMaterial(const TerrainMaterial& material) = 0;
	virtual void SetTerrainLight(const TerrainLight& light) = 0;
};