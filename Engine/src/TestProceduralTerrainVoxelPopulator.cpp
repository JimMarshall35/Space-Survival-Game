#include "TestProceduralTerrainVoxelPopulator.h"
#include "CommonTypedefs.h"
#include "IVoxelDataSource.h"
#include "ThreadPool.h"
#include "SimplexNoise.h"
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include "OctreeSerialisationLibrary.h"

TestProceduralTerrainVoxelPopulator::TestProceduralTerrainVoxelPopulator(const std::shared_ptr<rdx::thread_pool>& threadPool)
	:ThreadPool(threadPool)
{
}

void TestProceduralTerrainVoxelPopulator::PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo)
{
	std::unordered_set<TerrainOctreeIndex> octreeNodesSet;
	size_t availableWorkers = ThreadPool->NumWorkers();

	SimplexNoise noise;
	float maxHeight = 1000.0f;
	float planeHeight = 200.0f;

	u32 size = dataSrcToWriteTo->GetSize();
	glm::vec3 middle = glm::vec3(size/2,size/2,size/2);
	float radius = 500;
	for (i32 z = 1; z < size-1; z++)
	{
		for (i32 y = 1; y < size-1; y++)
		{
			for (i32 x = 1; x < size-1; x++)
			{
				float noiseVal = noise.fractal(3,x*0.001f,z*0.001f);
				float val = (planeHeight + noiseVal * 100.0f) - y;
				TerrainOctreeIndex indexSet = dataSrcToWriteTo->SetVoxelAt({ x,y,z }, std::clamp(-val, -127.0f, 127.0f)*3.0f);//std::clamp(y-noiseVal, -127.0f, 127.0f));
				if (octreeNodesSet.find(indexSet) == octreeNodesSet.end())
				{
					octreeNodesSet.insert(indexSet);
				}
			}
		}
		std::cout << "z: " << z << ". percent complete: " << (float)z / (float)(size - 1) * 100.0f << "%\n";
	}
	//OctreeSerialisation::SaveNewlyGeneratedToFile(octreeNodesSet, dataSrcToWriteTo, "test.vox");
}
