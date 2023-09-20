#include "SparseVoxelOctreeTestHelpers.h"
#include "SparseTerrainVoxelOctree.h"
#include "DefaultAllocator.h"
#include "Mocks.h"
#include <tuple>

namespace SparseOctreeTesttHelpers
{

	void GetTestObjects(OctreeAndMockDependencies& testObjectsOut, PreConstructionMockConfigurator configurator, u32 sizeVoxels, i8 clampValueHigh, i8 clampValueLow)
	{
		testObjectsOut.Polygonizer = std::make_unique<MockPolygonizer>();
		testObjectsOut.GraphicsAPIAdaptor = std::make_unique<MockTerrainGraphicsAPIAdaptor>();
		testObjectsOut.Allocator = std::make_unique<DefaultAllocator>();

		if (configurator)
		{
			// an opportunity to set up expectations of the mock dependencies before they are passed to the 
			// SparseTerrainVoxelOctree constructor (see gmock example: http://google.github.io/googletest/gmock_for_dummies.html, EXPECT_CALL ect)
			configurator(testObjectsOut);
		}

		testObjectsOut.Octree = std::make_unique<SparseTerrainVoxelOctree>(
			testObjectsOut.Allocator.get(),
			testObjectsOut.Polygonizer.get(),
			testObjectsOut.GraphicsAPIAdaptor.get(),
			sizeVoxels,
			clampValueHigh,
			clampValueLow);
	}
}
