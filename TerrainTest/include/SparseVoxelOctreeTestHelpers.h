#pragma once
#include <functional>
#include <memory>
#include "CommonTypedefs.h"
#include "SparseTerrainVoxelOctree.h"

class IAllocator;
class MockPolygonizer;
class MockTerrainGraphicsAPIAdaptor;
class SparseTerrainVoxelOctree;

namespace SparseOctreeTesttHelpers
{
	struct OctreeAndMockDependencies
	{
		std::unique_ptr<IAllocator> Allocator;
		std::unique_ptr<MockPolygonizer> Polygonizer;
		std::unique_ptr<MockTerrainGraphicsAPIAdaptor> GraphicsAPIAdaptor;
		// the order of members in this struct matters as Octree needs to use Allocator in its destructor!
		// this is maybe a sign that I should pass these dependencies in (and cache them) as shared ptrs.
		std::unique_ptr<SparseTerrainVoxelOctree> Octree;
	};

	typedef std::function<void(OctreeAndMockDependencies&)> PreConstructionMockConfigurator;

	void GetTestObjects(OctreeAndMockDependencies& testObjectsOut, PreConstructionMockConfigurator configurator = PreConstructionMockConfigurator(), u32 sizeVoxels = 64, i8 clampValueHigh = 50, i8 clampValueLow = -50);
}