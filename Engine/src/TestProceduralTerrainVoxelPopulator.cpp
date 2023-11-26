#include "TestProceduralTerrainVoxelPopulator.h"
#include "CommonTypedefs.h"
#include "IVoxelDataSource.h"
#include "ThreadPool.h"
#include "SimplexNoise.h"
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include "OctreeSerialisationLibrary.h"
#include <cstdarg>
#include <stdio.h>
#include <sstream>
#include "ITerrainOctreeNode.h"

TestProceduralTerrainVoxelPopulator::TestProceduralTerrainVoxelPopulator(const std::shared_ptr<rdx::thread_pool>& threadPool)
	:ThreadPool(threadPool)
{
}

static std::mutex sPrintMutex;
void ThreadsafePrint(const char* format, ...)
{
	std::lock_guard<std::mutex>lg(sPrintMutex);
	va_list args;
	va_start(args, format);
	vprintf_s(format, args);
}

void TestProceduralTerrainVoxelPopulator::PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo)
{
	ITerrainOctreeNode* onNode = dataSrcToWriteTo->GetParentNode();
	glm::ivec3 bl = onNode->GetBottomLeftCorner();
	int childDims = onNode->GetSizeInVoxels() / 2;
	SimplexNoise noise;
	float maxHeight = 1000.0f;
	float planeHeight = 200.0f;
	std::vector<std::future<void>> futures;
	for (i32 z = 0; z < 2; z++)
	{
		for (i32 y = 0; y < 2; y++)
		{
			for (i32 x = 0; x < 2; x++)
			{
				i32 i = x + (2 * y) + (4 * z);
				glm::ivec3& childBL =
					glm::ivec3{
						bl.x + x * childDims,
						bl.y + y * childDims,
						bl.z + z * childDims
				};
				
				futures.push_back(ThreadPool->enqueue([&, childBL]() {
					std::ostringstream id;
					std::string sid;

					if (sid.empty())
					{
						id << std::this_thread::get_id();
						sid = id.str();
					}
					for (int tz = childBL.z; tz < childBL.z + childDims; tz++)
					{
						for (int ty = childBL.y; ty < childBL.y + childDims; ty++)
						{
							for (int tx = childBL.x; tx < childBL.x + childDims; tx++)
							{
								float noiseVal = noise.fractal(3, tx * 0.001f, tz * 0.001f);
								float val = (planeHeight + noiseVal * 100.0f) - ty;
								TerrainOctreeIndex indexSet = dataSrcToWriteTo->SetVoxelAt({ tx,ty,tz }, std::clamp(-val, -127.0f, 127.0f));
							}
						}
						ThreadsafePrint("thread ID %s. Z: %i. Complete: %f\n", sid.c_str(), tz, ((float)(tz - childBL.z) / (float)childDims) * 100.0f);
					}
				}));
			}
		}
	}
	for (auto& future : futures)
	{
		future.wait();
	}
	//std::unordered_set<TerrainOctreeIndex> octreeNodesSet;
	//size_t availableWorkers = ThreadPool->NumWorkers();

	//SimplexNoise noise;
	//float maxHeight = 1000.0f;
	//float planeHeight = 200.0f;

	//u32 size = dataSrcToWriteTo->GetSize();
	//glm::vec3 middle = glm::vec3(size/2,size/2,size/2);
	//float radius = 500;
	//i32 step = size / availableWorkers;
	//std::vector<std::future<void>> futures;
	//for (i32 threadStartZ = 0; threadStartZ < size; threadStartZ += step)
	//{
	//	futures.push_back(ThreadPool->enqueue([&]() {
	//		static std::ostringstream id;
	//		static std::string sid;

	//		if (sid.empty())
	//		{
	//			id << std::this_thread::get_id();
	//			sid = id.str();
	//		}

	//		for (i32 z = threadStartZ; z < threadStartZ + step; z++)
	//		{
	//			for (i32 y = 0; y < size; y++)
	//			{
	//				for (i32 x = 0; x < size; x++)
	//				{
	//					float noiseVal = noise.fractal(3, x * 0.001f, z * 0.001f);
	//					float val = (planeHeight + noiseVal * 100.0f) - y;
	//					TerrainOctreeIndex indexSet = dataSrcToWriteTo->SetVoxelAt({ x,y,z }, std::clamp(-val, -127.0f, 127.0f));//std::clamp(y-noiseVal, -127.0f, 127.0f));

	//				}
	//			}

	//			ThreadsafePrint("thread ID %s. Z: %i. Complete: %f%\n", sid, z, ((float)z - (float)threadStartZ) / (float)step);
	//		}
	//		
	//	}));
	//	
	//}
	//for (auto& future : futures)
	//{
	//	future.wait();
	//}
}
