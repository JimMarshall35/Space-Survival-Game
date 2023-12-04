#include "TestProceduralTerrainVoxelPopulator.h"
#include "CommonTypedefs.h"
#include "IVoxelDataSource.h"
#include "ThreadPool.h"
#include "SimplexNoise.h"
#include <algorithm>
#include <iostream>
#include "OctreeSerialisationLibrary.h"
#include <cstdarg>
#include <stdio.h>
#include <sstream>
#include "ITerrainOctreeNode.h"
#include <chrono>

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

static int numDecks = 8*8*512;
static int decksCompleted = 0;
void OnDeckCompleted()
{
	std::lock_guard<std::mutex>lg(sPrintMutex);
	printf("%f percent complete\n", ((float)++decksCompleted / (float)numDecks) * 100.0f);
}

void TestProceduralTerrainVoxelPopulator::PopulateSingleNode(IVoxelDataSource* dataSrcToWriteTo, ITerrainOctreeNode* node, SimplexNoise& noise, std::unordered_set<TerrainOctreeIndex>* output )
{
	glm::ivec3 childBL = node->GetBottomLeftCorner();
	int childDims = node->GetSizeInVoxels();
	float planeHeight = 200.0f;

	for (int tz = childBL.z; tz < childBL.z + childDims; tz++)
	{
		for (int ty = childBL.y; ty < childBL.y + childDims; ty++)
		{
			for (int tx = childBL.x; tx < childBL.x + childDims; tx++)
			{
				float noiseVal = noise.fractal(4, 2.0*tx * 0.001f,  ty * 0.0001f, 2.0*tz * 0.001f);
				float val = (planeHeight + noiseVal * 200.0f) - ty;
				TerrainOctreeIndex indexSet = dataSrcToWriteTo->SetVoxelAt({ tx,ty,tz }, std::clamp(-val*10.0f, -127.0f, 127.0f));
			}
		}
		OnDeckCompleted();
	}
}


void TestProceduralTerrainVoxelPopulator::PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo)
{
	//OctreeSerialisation::LoadFromFile(dataSrcToWriteTo,"level.vox");
	//return;

	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::milliseconds;

	auto t1 = high_resolution_clock::now();

	ITerrainOctreeNode* onNode = dataSrcToWriteTo->GetParentNode();
	int childDims = onNode->GetSizeInVoxels() / 2;
	SimplexNoise noise;
	float maxHeight = 1000.0f;
	float planeHeight = 200.0f;
	std::vector<std::future<void>> futures;
	std::unordered_set<TerrainOctreeIndex> cellsWrittenTo[8];
	decksCompleted = 0;
	if (ThreadPool->NumWorkers() > 8)
	{
		// if we have more than 8 threads then queue a task for each of the first 2 mip levels,
		// 64 nodes in total
		numDecks = 8*8*512;
		
		dataSrcToWriteTo->CreateChildrenForFirstNMipLevels(onNode, 2);
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				ITerrainOctreeNode* child = onNode->GetChild(i)->GetChild(j);
				futures.push_back(ThreadPool->enqueue([&,child,dataSrcToWriteTo]() {
					PopulateSingleNode(dataSrcToWriteTo, child, noise, &cellsWrittenTo[i]);
				}));
			}
		}
	}
	else
	{
		// if we have <= 8 workers then queue 8 nodes to be generated
		numDecks = 8*1024;
		dataSrcToWriteTo->CreateChildrenForFirstNMipLevels(onNode, 1);
		for (int i = 0; i < 8; i++)
		{
			ITerrainOctreeNode* child = onNode->GetChild(i);
			futures.push_back(ThreadPool->enqueue([&,child,dataSrcToWriteTo]() {
				PopulateSingleNode(dataSrcToWriteTo, child, noise, &cellsWrittenTo[i]);
			}));
		}
	}
	

	for (auto& future : futures)
	{
		future.wait();
	}

	auto t2 = high_resolution_clock::now();

	/* Getting number of milliseconds as an integer. */
	auto ms_int = duration_cast<milliseconds>(t2 - t1);

	std::cout << "done in " << ms_int.count() << "ms\n";

	//OctreeSerialisation::SaveNewlyGeneratedToFile(allThreads, dataSrcToWriteTo, "level.vox");
}
