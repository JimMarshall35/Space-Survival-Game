#include "pch.h"
#include "SparseVoxelOctreeTestHelpers.h"
#include "Mocks.h"
#include "SparseTerrainVoxelOctree.h"
#include "DefaultAllocator.h"
#include <random>
#include <iostream>


const u32 gSizeVoxels = 2048;
const i8 gClampMin = -127;
const i8 gClampMax = 127;


#define RNGTestSetupBoilerPlate std::random_device rd; std::mt19937 gen(rd()); std::mt19937 voxelGen(rd()); std::uniform_int_distribution<unsigned int> posDistr(0, gSizeVoxels-1); std::uniform_int_distribution<int> voxlDistr(gClampMin, gClampMax); // define the range

TEST(SparseTerrainVoxelOctree, RandomStoreAndRetrieve)
{
	// arrange
	RNGTestSetupBoilerPlate;

	using namespace SparseOctreeTesttHelpers;
	OctreeAndMockDependencies objects;
	GetTestObjects(objects, PreConstructionMockConfigurator(), gSizeVoxels, gClampMax, gClampMin);
	SparseTerrainVoxelOctree& octree = *objects.Octree.get();

	glm::ivec3 locToWrite = { posDistr(gen), posDistr(gen), posDistr(gen) };
	i8 valueToWrite = voxlDistr(voxelGen);
	std::cerr << "valueToWrite: " << valueToWrite << "\n";
	std::cerr << "locationToWrite: { " << locToWrite.x <<", "<<  
										locToWrite.y << ", " << 
										locToWrite.z << " }\n";
	// act
	octree.SetVoxelAt(locToWrite, valueToWrite);
	i8 valueRead = octree.GetVoxelAt(locToWrite);

	// assert
	ASSERT_EQ(valueToWrite, valueRead);
}

TEST(SparseTerrainVoxelOctree, RandomStoreAndRetrieve100Times)
{
	// arrange
	RNGTestSetupBoilerPlate;

	using namespace SparseOctreeTesttHelpers;
	OctreeAndMockDependencies objects;
	GetTestObjects(objects, PreConstructionMockConfigurator(), gSizeVoxels, gClampMax, gClampMin);
	SparseTerrainVoxelOctree& octree = *objects.Octree.get();

	const u32 timesToRepeat = 100;

	glm::ivec3 positionsToWrite[timesToRepeat];
	i8 valuesToWrite[timesToRepeat];

	// act
	std::cerr << "Writing voxels...\n";
	for (int i = 0; i < timesToRepeat; i++)
	{
		glm::ivec3 positionToWrite = { posDistr(gen), posDistr(gen), posDistr(gen) };
		i8 valueToWrite = voxlDistr(voxelGen);
		positionsToWrite[i] = positionToWrite;
		valuesToWrite[i] = valueToWrite;
		std::cerr << "valueToWrite: " << valueToWrite << ", ";
		std::cerr << "locationToWrite: { " << positionToWrite.x << ", " <<
			positionToWrite.y << ", " <<
			positionToWrite.z << " }\n";
		octree.SetVoxelAt(positionToWrite, valueToWrite);
	}


	// assert
	for (int i = 0; i < timesToRepeat; i++)
	{
		glm::ivec3 positionToWrite = positionsToWrite[i];
		i8 valueWrote = valuesToWrite[i];
		i8 valueRead = octree.GetVoxelAt(positionToWrite);
		EXPECT_EQ(valueWrote, valueRead) << "locationToWrite: { " << positionToWrite.x << ", " << // EXPECT_EQ is different from ASSERT_EQ in that it doesn't terminate the test if it fails
			positionToWrite.y << ", " <<
			positionToWrite.z << " }\n";
	}
}