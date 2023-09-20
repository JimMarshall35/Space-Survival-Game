#include "pch.h"
#include "Mocks.h"
#include "SparseTerrainVoxelOctree.h"
#include "DefaultAllocator.h"
#include <tuple>
#include <memory>
#include <functional>



struct OctreeAndMockDependencies
{
	std::unique_ptr<IAllocator> Allocator;
	std::unique_ptr<MockPolygonizer> Polygonizer;
	std::unique_ptr<MockTerrainGraphicsAPIAdaptor> GraphicsAPIAdaptor;
	// the order of members in this struct matters as Octree needs to use Allocator in its destructor!
	std::unique_ptr<SparseTerrainVoxelOctree> Octree;
};

typedef std::function<void(OctreeAndMockDependencies&)> PreConstructionMockConfigurator;

namespace OctreeTesttHelpers
{
	void GetTestObjects(OctreeAndMockDependencies& testObjectsOut, PreConstructionMockConfigurator configurator = PreConstructionMockConfigurator(), u32 sizeVoxels=64, i8 clampValueHigh=50, i8 clampValueLow=-50)
	{
		testObjectsOut.Polygonizer = std::make_unique<MockPolygonizer>();
		testObjectsOut.GraphicsAPIAdaptor = std::make_unique<MockTerrainGraphicsAPIAdaptor>();
		testObjectsOut.Allocator = std::make_unique<DefaultAllocator>();
		testObjectsOut.Octree = std::make_unique<SparseTerrainVoxelOctree>(
			testObjectsOut.Allocator.get(),
			testObjectsOut.Polygonizer.get(),
			testObjectsOut.GraphicsAPIAdaptor.get(),
			sizeVoxels,
			clampValueHigh,
			clampValueLow);

		if (configurator)
		{
			configurator(testObjectsOut);
		}
	}
}

TEST(SparseTerrainVoxelOctree, Test1)
{
	OctreeAndMockDependencies objects;
	OctreeTesttHelpers::GetTestObjects(objects);
}

int main(int argc, char** argv)
{
	// This allows us to call this executable with various command line arguments
	// which get parsed in InitGoogleTest
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
