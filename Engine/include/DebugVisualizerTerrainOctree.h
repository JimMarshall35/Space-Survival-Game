//#pragma once
//#include "OctreeTypes.h"
//#include "ITerrainOctree.h"
//#include "ITerrainOctreeNode.h"
//#include <memory>
//#include <glm.hpp>
//#include <vector>
//#include "Camera.h"
//#include "Core.h"
//
//
//using namespace glm;
//
//
//
//
//class APP_API DebugVisualizerTerrainOctree : public ITerrainOctree
//{
//private:
//
//	struct TerrainOctreeNode : public ITerrainOctreeNode
//	{
//		std::unique_ptr<TerrainOctreeNode> Children[8];
//		u32 MipLevel;
//		ivec3 BottomLeftCorner;
//		u32 SizeInVoxels;
//		TerrainChunkMesh Mesh;
//		virtual ITerrainOctreeNode* GetChild(u8 child)const override;// { return Children[child].get(); }
//		virtual const ivec3& GetBottomLeftCorner()const override;// { return BottomLeftCorner; }
//		virtual u32 GetSizeInVoxels() const override;// { return SizeInVoxels; }
//		virtual u32 GetMipLevel() const override;// { return MipLevel; }
//		virtual const TerrainChunkMesh& GetTerrainChunkMesh() const override;
//		virtual void SetTerrainChunkMesh(const TerrainChunkMesh& mesh) override;
//		virtual bool NeedsRegenerating() const override { return false; }
//		virtual TerrainChunkMesh& GetTerrainChunkMeshMutable() override { return Mesh; };
//	};
//
//public:
//
//	DebugVisualizerTerrainOctree();
//
//	// initialise octree
//	void MakeQuadtree();
//
//	// return a list of TerrainOctreeNodes to render.
//	// these will be frustum culled and LOD'd correctly 
//	// from the cam data provided
//	void GetChunksToRender(
//		const Camera& camera,
//		float aspect, float fovY, float zNear, float zFar,
//		std::vector<ITerrainOctreeNode*>& outNodesToRender);
//
//	// draw a gizmo box around each chunk in list
//	void DebugVisualiseChunks(const std::vector<ITerrainOctreeNode*>& nodes);
//
//	bool bDebugFill = false;
//
//	i32 DebugMipLevelToDraw = -1;
//
//	f32 GetMinimumViewportAreaThreshold();// { return LODCulling.MinimumViewportAreaThreshold; }
//
//	void SetMinimumViewportAreaThreshold(f32 val);// { LODCulling.MinimumViewportAreaThreshold = val; }
//
//private:
//
//	// recursive function called from MakeQuadtree
//	void PopulateChildren(TerrainOctreeNode* node);
//
//	void PopulateDebugMipLevelColourTable(u32 maximumMipLevel);
//
//	void DebugVisualiseMipLevel(TerrainOctreeNode* parentNode, u32 mipLevel, const glm::vec4& colour);
//
//private:
//	
//	// parent node of the octree representing the entire voxel volume.
//	// children are chunks of half size, one in each quadrant of their parent.
//	TerrainOctreeNode ParentNode;
//
//	// for colouring different mip level blocks different colours during debugging
//	std::unique_ptr<glm::vec4[]> DebugMipLevelColourTable;
//
//	bool bDebugCameraSet;
//	
//	Camera DebugCamera;
//};