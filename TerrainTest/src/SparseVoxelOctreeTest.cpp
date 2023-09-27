#include "pch.h"
#include "SparseVoxelOctreeTestHelpers.h"
#include "Mocks.h"
#include "SparseTerrainVoxelOctree.h"
#include "DefaultAllocator.h"
#include "TerrainDefs.h"
#include <random>
#include <iostream>
#include <fstream>

const u32 gSizeVoxels = 512;
const i8 gClampMin = -127;
const i8 gClampMax = 127;


#define RNGTestSetupBoilerPlate std::random_device rd; \
unsigned int positionSeed = rd();\
unsigned int voxelSeed = rd();\
std::mt19937 gen(positionSeed);\
 std::mt19937 voxelGen(voxelSeed);\
 std::uniform_int_distribution<unsigned int> posDistr(0, gSizeVoxels-1);\
 std::uniform_int_distribution<int> voxlDistr(gClampMin, gClampMax);\
std::cerr << "Position Seed: "<< positionSeed << "Voxel Seed: "<< voxelSeed << "\n";

#define RNGTestSetupBoilerPlateWithSeed(positionSeed, voxelSeed) std::mt19937 gen(positionSeed);\
 std::mt19937 voxelGen(voxelSeed);\
 std::uniform_int_distribution<unsigned int> posDistr(0, gSizeVoxels-1);\
 std::uniform_int_distribution<int> voxlDistr(gClampMin, gClampMax);\
std::cerr << "Position Seed: "<< positionSeed << "Voxel Seed: "<< voxelSeed << "\n";


static void LogFailedTestSeeds(u32 posSeed, u32 voxSeed, const std::string& filePath)
{
	std::ofstream outfile;

	outfile.open(filePath, std::ofstream::out | std::ofstream::app); // append instead of overwrite
	outfile << posSeed << " " << voxSeed << "\n";

}

static void StoreAndRetrieveBaseTest(const std::string& failedTestSeedLogFile, int timesToRepeat, bool verbose=false, bool isReplay = false, u32 posSeed = 0, u32 voxSeed = 0)
{
	// arrange
	std::random_device rd;
	posSeed = isReplay ? posSeed : rd();
	voxSeed = isReplay ? voxSeed : rd();

	RNGTestSetupBoilerPlateWithSeed(posSeed, voxSeed);

	using namespace SparseOctreeTesttHelpers;
	OctreeAndMockDependencies objects;
	GetTestObjects(objects, PreConstructionMockConfigurator(), gSizeVoxels, gClampMax, gClampMin);
	SparseTerrainVoxelOctree& octree = *objects.Octree.get();

	std::vector<glm::ivec3> positionsToWrite(timesToRepeat);
	std::vector<i8> valuesToWrite(timesToRepeat);
	auto hasAlreadyWrittenThisPosition = [&positionsToWrite](const glm::ivec3& positionToWrite, int numWrittenSoFar) -> bool
	{
		for (int i = 0; i < numWrittenSoFar; i++)
		{
			if (positionsToWrite[i] == positionToWrite)
			{
				return true;
			}
		}
		return false;
	};
	// act
	if (verbose)
	{
		std::cerr << "Writing voxels...\n";
	}
	
	for (int i = 0; i < timesToRepeat; i++)
	{
		glm::ivec3 positionToWrite = { posDistr(gen), posDistr(gen), posDistr(gen) };
		i8 valueToWrite = voxlDistr(voxelGen);
		positionsToWrite[i] = positionToWrite;
		valuesToWrite[i] = valueToWrite;
		while (hasAlreadyWrittenThisPosition(positionToWrite, i))
		{
			positionToWrite = { posDistr(gen), posDistr(gen), posDistr(gen) };
		}
		if (verbose)
		{
			std::cerr << "valueToWrite: " << valueToWrite << ", ";
			std::cerr << "locationToWrite: { " << positionToWrite.x << ", " <<
				positionToWrite.y << ", " <<
				positionToWrite.z << " }\n";
		}
		
		octree.SetVoxelAt(positionToWrite, valueToWrite);
	}


	// assert
	bool testPassed = true;
	for (int i = 0; i < timesToRepeat; i++)
	{
		glm::ivec3 positionToWrite = positionsToWrite[i];
		i8 valueWrote = valuesToWrite[i];
		i8 valueRead = octree.GetVoxelAt(positionToWrite);
		if (isReplay)
		{
			if (valueWrote != valueRead)
			{
				std::cerr << "locationToWrite: { " << positionToWrite.x << ", " <<
					positionToWrite.y << ", " <<
					positionToWrite.z << " } Expected: " << (int)valueWrote << " Got: "<< (int)valueRead << "\n";
			}
		}
		else
		{
			EXPECT_EQ(valueWrote, valueRead) << "locationToWrite: { " << positionToWrite.x << ", " << // EXPECT_EQ is different from ASSERT_EQ in that it doesn't terminate the test if it fails
				positionToWrite.y << ", " <<
				positionToWrite.z << " }\n";
		}
		
		if (testPassed && (valueWrote != valueRead))
		{
			testPassed = false;
		}
	}
	if (!testPassed && !isReplay)
	{
		LogFailedTestSeeds(posSeed, voxSeed, failedTestSeedLogFile);
	}
}

TEST(SparseTerrainVoxelOctree, RandomStoreAndRetrieve)
{
	StoreAndRetrieveBaseTest("FailedTestSeedValues/RandomStoreAndRetrieve.txt", 1);
}

TEST(SparseTerrainVoxelOctree, RandomStoreAndRetrieve100Times)
{
	StoreAndRetrieveBaseTest("FailedTestSeedValues/RandomStoreAndRetrieve100Times.txt", 100);
}

TEST(SparseTerrainVoxelOctree, RandomStoreAndRetrieve10000Times)
{
	StoreAndRetrieveBaseTest("FailedTestSeedValues/RandomStoreAndRetrieve10000Times.txt", 10000);
}

TEST(SparseTerrainVoxelOctree, ReplayFailed_RandomStoreAndRetrieve10000Times)
{
	std::ifstream stream;
	stream.open("FailedTestSeedValues/RandomStoreAndRetrieve10000Times.txt");
	unsigned int positionSeed, voxelSeed;
	unsigned int onTest = 0;
	while (stream >> positionSeed >> voxelSeed)
	{
		std::cerr << "Rerunning failed test " << onTest++ << "\n";
		StoreAndRetrieveBaseTest("FailedTestSeedValues/RandomStoreAndRetrieve10000Times.txt", 10000, 
			false, // verbose
			true,  // isReplay
			positionSeed, voxelSeed);
	}
}

TEST(SparseTerrainVoxelOctree, GetVoxelsForRandomNode)
{
	// arrange
	std::random_device rd; std::mt19937 gen(rd()); 
	std::mt19937 voxelGen(rd()); 
	std::uniform_int_distribution<unsigned int> posDistr(POLYGONIZER_NEGATIVE_GUTTER, gSizeVoxels - 1 - POLYGONIZER_POSITIVE_GUTTER - BASE_CELL_SIZE);
	std::uniform_int_distribution<int> voxlDistr(gClampMin, gClampMax); // define the range

	using namespace SparseOctreeTesttHelpers;
	OctreeAndMockDependencies objects;
	glm::ivec3 bottomLeft = { posDistr(gen), posDistr(gen), posDistr(gen) };

	i8 inVoxels[TOTAL_CELL_VOLUME_SIZE];

	


	GetTestObjects(objects, PreConstructionMockConfigurator(), gSizeVoxels, gClampMax, gClampMin);

	i32 i = 0;
	SparseTerrainVoxelOctree& octree = *objects.Octree.get();
	for (int z = bottomLeft.z; z < bottomLeft.z + TOTAL_CELL_SIZE; z++)
	{
		for (int y = bottomLeft.y; y < bottomLeft.y + TOTAL_CELL_SIZE; y++)
		{
			for (int x = bottomLeft.x; x < bottomLeft.x + TOTAL_CELL_SIZE; x++)
			{
				//i32 i = x + (2 * y) + (4 * z); 1500 113 228
				i8 randomVoxel = voxlDistr(voxelGen);
				inVoxels[i++] = randomVoxel;
				octree.SetVoxelAt({ x,y,z }, randomVoxel);
			}
		}
	}

	i8 outVoxels[TOTAL_CELL_VOLUME_SIZE];
	MockTerrainOctreeNode node;
	EXPECT_CALL(node, GetBottomLeftCorner())
		.WillOnce([bottomLeft]() { return bottomLeft + glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER, POLYGONIZER_NEGATIVE_GUTTER, POLYGONIZER_NEGATIVE_GUTTER); });

	EXPECT_CALL(node, GetSizeInVoxels())
		.WillOnce([]() { return 16; });

	octree.GetVoxelsForNode(&node, outVoxels);

	for (int i = 0; i < TOTAL_CELL_VOLUME_SIZE; i++)
	{
		ASSERT_EQ(outVoxels[i], inVoxels[i]) << "i was "<< i;
	}
}