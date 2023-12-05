#include "TerrainPolygonizer.h"
#include "IAllocator.h"
#include "TerrainDefs.h"
#include "IVoxelDataSource.h"
#include "TransVoxel.h"
#include "ITerrainOctreeNode.h"
#include <cmath>

#define TERRAIN_CELL_VERTEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum
#define TERRAIN_CELL_INDEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum

#define TERRAIN_CELL_TRANSITION_MESH_VERTEX_ARRAY_SIZE 1000
#define TERRAIN_CELL_TRANSITION_MESH_INDEX_ARRAY_SIZE 1000

#define TERRAIN_FIXED_FRACTION_SIZE_BITS 8
#define TERRAIN_FIXED_FRACTION_MAX 0xff

#define K   (1 << (Q - 1))

TerrainPolygonizer::TerrainPolygonizer(IAllocator* allocator, std::shared_ptr<rdx::thread_pool> threadPool)
	:Allocator(allocator),
	ThreadPool(threadPool)
{
	size_t threadPoolSize = std::thread::hardware_concurrency();
}

TerrainPolygonizer::~TerrainPolygonizer()
{
	
}

std::future<PolygonizeWorkerThreadData*> TerrainPolygonizer::PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source)
{
	auto r = ThreadPool->enqueue([this, node, source]() {
		return PolygonizeCellSync(node, source);
	});
	return r;
}

//marching cubes table data
int edgeTable[256]={
0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };
char triTable[256][16] =
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};



//#pragma optimize("", off)
/*
   Linearly interpolate the position where an isosurface cuts
   an edge between two vertices, each with their own scalar value
*/
TerrainVertex TerrainPolygonizer::VertexInterp( glm::vec3 p1, glm::vec3 p2,float valp1,float valp2, glm::ivec3& coords1, glm::ivec3& coords2, i8* voxels, ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source)
{
	auto GetVoxelValueAt = [](u8 x, u8 y, u8 z, i8* voxels) -> i8
	{
		return voxels[TOTAL_DECK_SIZE * z + TOTAL_CELL_SIZE * y + x];
	};
	glm::vec3 originalp1 = p1;
	glm::vec3 originalp2 = p2;
	TerrainVertex v;
	float mu = -valp1 / (valp2 - valp1);
	u32 mipLevel = cellToPolygonize->GetMipLevel();
	float dims = cellToPolygonize->GetSizeInVoxels();
	glm::vec3 normal1 = glm::normalize(glm::vec3{
		(float)GetVoxelValueAt(coords1.x - 1, coords1.y, coords1.z,voxels) - (float)GetVoxelValueAt(coords1.x + 1, coords1.y, coords1.z,voxels),
		(float)GetVoxelValueAt(coords1.x, coords1.y - 1, coords1.z,voxels) - (float)GetVoxelValueAt(coords1.x, coords1.y + 1, coords1.z,voxels),
		(float)GetVoxelValueAt(coords1.x, coords1.y, coords1.z - 1,voxels) - (float)GetVoxelValueAt(coords1.x, coords1.y, coords1.z + 1,voxels)
	});
	glm::vec3 normal2 = glm::normalize(glm::vec3{
		(float)GetVoxelValueAt(coords2.x - 1, coords2.y, coords2.z,voxels) - (float)GetVoxelValueAt(coords2.x + 1, coords2.y, coords2.z,voxels),
		(float)GetVoxelValueAt(coords2.x, coords2.y - 1, coords2.z,voxels) - (float)GetVoxelValueAt(coords2.x, coords2.y + 1, coords2.z,voxels),
		(float)GetVoxelValueAt(coords2.x, coords2.y, coords2.z - 1,voxels) - (float)GetVoxelValueAt(coords2.x, coords2.y, coords2.z + 1,voxels)
	});
	if (mipLevel == 0 || !bExactFit)
	{
		/*glm::vec3 normal1 = glm::normalize(glm::vec3{
			(float)GetVoxelValueAt(coords1.x - 1, coords1.y, coords1.z,voxels) - (float)GetVoxelValueAt(coords1.x + 1, coords1.y, coords1.z,voxels),
			(float)GetVoxelValueAt(coords1.x, coords1.y - 1, coords1.z,voxels) - (float)GetVoxelValueAt(coords1.x, coords1.y + 1, coords1.z,voxels),
			(float)GetVoxelValueAt(coords1.x, coords1.y, coords1.z - 1,voxels) - (float)GetVoxelValueAt(coords1.x, coords1.y, coords1.z + 1,voxels)
		});
		glm::vec3 normal2 = glm::normalize(glm::vec3{
			(float)GetVoxelValueAt(coords2.x - 1, coords2.y, coords2.z,voxels) - (float)GetVoxelValueAt(coords2.x + 1, coords2.y, coords2.z,voxels),
			(float)GetVoxelValueAt(coords2.x, coords2.y - 1, coords2.z,voxels) - (float)GetVoxelValueAt(coords2.x, coords2.y + 1, coords2.z,voxels),
			(float)GetVoxelValueAt(coords2.x, coords2.y, coords2.z - 1,voxels) - (float)GetVoxelValueAt(coords2.x, coords2.y, coords2.z + 1,voxels)
		});
		*/
		v.Position =  (p1 + mu * (p2 - p1));
		v.Normal = glm::normalize(normal1 + mu * (normal2 - normal1));
	}
	else
	{
		
		float oldMu = mu;
		while (mipLevel > 0)
		{
			glm::ivec3 midPoint = glm::vec3(p1) + glm::vec3(p2 - p1) * 0.5f;
			float midPointVal = source->GetVoxelAt(midPoint);
			if (mu > 0.5f)
			{
				
				p1 = midPoint;
				valp1 = midPointVal;
			}
			else if (mu < 0.5f)
			{
				p2 = midPoint;
				valp2 = midPointVal;
			}
			else
			{
				break;
			}

			mu = mu > 0.5f ? (mu-0.5f)/0.5f : (mu/0.5f);//-valp1 / (valp2 - valp1);
			--mipLevel;
		}
		assert(((valp1 <= 0) && (valp2 >= 0)) || (valp1 >= 0 && valp2 <= 0));
		/*glm::vec3 normal1 = glm::normalize(glm::vec3{
			source->GetVoxelAt(p1 + glm::vec3{-1,0,0}) - source->GetVoxelAt(p1 + glm::vec3{1,0,0}),
			source->GetVoxelAt(p1 + glm::vec3{0,-1,0}) - source->GetVoxelAt(p1 + glm::vec3{0,1,0}),
			source->GetVoxelAt(p1 + glm::vec3{0,0,-1}) - source->GetVoxelAt(p1 + glm::vec3{0,0,1})

		});
		glm::vec3 normal2 = glm::normalize(glm::vec3{
			source->GetVoxelAt(p2 + glm::vec3{-1,0,0}) - source->GetVoxelAt(p2 + glm::vec3{1,0,0}),
			source->GetVoxelAt(p2 + glm::vec3{0,-1,0}) - source->GetVoxelAt(p2 + glm::vec3{0,1,0}),
			source->GetVoxelAt(p2 + glm::vec3{0,0,-1}) - source->GetVoxelAt(p2 + glm::vec3{0,0,1})
		});*/
		v.Position =  (p1 + mu * (p2 - p1));
		v.Normal = glm::normalize(normal1 + mu * (normal2 - normal1));
	}

	return v;
}
//#pragma optimize("", on)

int TerrainPolygonizer::Polygonise(GridCell &Grid, int &NewVertexCount, TerrainVertex *Vertices, int& newIndicesCount, char* indices, i8* voxels, ITerrainOctreeNode* node, IVoxelDataSource* source)
{
	int CubeIndex;

	TerrainVertex VertexList[12];
	TerrainVertex NewVertexList[12];
	char LocalRemap[12];
	
	//Determine the index into the edge table which
	//tells us which vertices are inside of the surface
	CubeIndex = 0;
	if (Grid.val[0] < 0.0f) CubeIndex |= 1;
	if (Grid.val[1] < 0.0f) CubeIndex |= 2;
	if (Grid.val[2] < 0.0f) CubeIndex |= 4;
	if (Grid.val[3] < 0.0f) CubeIndex |= 8;
	if (Grid.val[4] < 0.0f) CubeIndex |= 16;
	if (Grid.val[5] < 0.0f) CubeIndex |= 32;
	if (Grid.val[6] < 0.0f) CubeIndex |= 64;
	if (Grid.val[7] < 0.0f) CubeIndex |= 128;

	//Cube is entirely in/out of the surface
	if (edgeTable[CubeIndex] == 0)
		return(0);

	//Find the vertices where the surface intersects the cube
	if (edgeTable[CubeIndex] & 1) {
		VertexList[0] = VertexInterp(Grid.p[0],Grid.p[1],Grid.val[0],Grid.val[1],Grid.coords[0],Grid.coords[1],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 2) {
		VertexList[1] = VertexInterp(Grid.p[1],Grid.p[2],Grid.val[1],Grid.val[2],Grid.coords[1],Grid.coords[2],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 4) {
		VertexList[2] = VertexInterp(Grid.p[2],Grid.p[3],Grid.val[2],Grid.val[3],Grid.coords[2],Grid.coords[3],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 8) {
		VertexList[3] = VertexInterp(Grid.p[3],Grid.p[0],Grid.val[3],Grid.val[0],Grid.coords[3],Grid.coords[0],voxels, node, source);
	}

	if (edgeTable[CubeIndex] & 16) {
		VertexList[4] = VertexInterp(Grid.p[4],Grid.p[5],Grid.val[4],Grid.val[5],Grid.coords[4],Grid.coords[5],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 32) {
		VertexList[5] = VertexInterp(Grid.p[5],Grid.p[6],Grid.val[5],Grid.val[6],Grid.coords[5],Grid.coords[6],voxels, node, source);
	}

	if (edgeTable[CubeIndex] & 64) {
		VertexList[6] = VertexInterp(Grid.p[6],Grid.p[7],Grid.val[6],Grid.val[7],Grid.coords[6],Grid.coords[7],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 128) {
		VertexList[7] = VertexInterp(Grid.p[7],Grid.p[4],Grid.val[7],Grid.val[4],Grid.coords[7],Grid.coords[4],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 256) {
		VertexList[8] = VertexInterp(Grid.p[0],Grid.p[4],Grid.val[0],Grid.val[4],Grid.coords[0],Grid.coords[4],voxels, node, source);
	}

	if (edgeTable[CubeIndex] & 512) {
		VertexList[9] = VertexInterp(Grid.p[1],Grid.p[5],Grid.val[1],Grid.val[5],Grid.coords[1],Grid.coords[5],voxels, node, source);
	}

	if (edgeTable[CubeIndex] & 1024) {
		VertexList[10] = VertexInterp(Grid.p[2],Grid.p[6],Grid.val[2],Grid.val[6],Grid.coords[2],Grid.coords[6],voxels, node, source);
	}
		
	if (edgeTable[CubeIndex] & 2048) {
		VertexList[11] = VertexInterp(Grid.p[3],Grid.p[7],Grid.val[3],Grid.val[7],Grid.coords[3],Grid.coords[7],voxels, node, source);
	}
		

	NewVertexCount=0;
	for (u32 i=0;i<12;i++)
		LocalRemap[i] = -1;

	for (u32 i=0;triTable[CubeIndex][i]!=-1;i++)
	{
		if(LocalRemap[triTable[CubeIndex][i]] == -1)
		{
			NewVertexList[NewVertexCount] = VertexList[triTable[CubeIndex][i]];
			LocalRemap[triTable[CubeIndex][i]] = NewVertexCount;
			NewVertexCount++;
		}
	}

	for (int i=0;i<NewVertexCount;i++) {
		Vertices[i].Position = NewVertexList[i].Position;
		Vertices[i].Normal = NewVertexList[i].Normal;
	}

	newIndicesCount = 0;
	for (u32 i=0;triTable[CubeIndex][i]!=-1;i+=3) {
		indices[newIndicesCount++] = LocalRemap[triTable[CubeIndex][i+0]];
		indices[newIndicesCount++] = LocalRemap[triTable[CubeIndex][i+1]];
		indices[newIndicesCount++] = LocalRemap[triTable[CubeIndex][i+2]];
	}

	return(newIndicesCount);
}



PolygonizeWorkerThreadData* TerrainPolygonizer::PolygonizeCellSync(ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source)
{

	/*
	For each cell, we must have space to store four vertex indexes corresponding to the locations
	shown in Figure 3.8(a) so that they can be reused by cells triangulated at a later time. It is never
	the case that all four vertex slots are used at once (because a vertex lying on the interior of an
	edge implies no vertex at the corner), but we must be able to identify the vertices using a fixed
	indexing scheme. The vertices used by a cell are always owned by the cell itself, the preceding
	cell in the same row, one of two adjacent cells in the preceding row, or one of four cells in the
	preceding deck. We can therefore limit the vertex history that we store while processing a block
	to two decks containing 16 16 cells each and ping-pong between them as the z coordinate is in-
	cremented
	*/

	return PolygonizeCellSyncMMC(cellToPolygonize, source);
	
	u32 cellOutputTop = 0;

	u8* data = (u8*)Allocator->Malloc(
		sizeof(PolygonizeWorkerThreadData) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainVertex) +
		TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32) +
		TOTAL_CELL_VOLUME_SIZE * sizeof(i8)
	); // malloc everything in a single block

	PolygonizeWorkerThreadData* rVal = (PolygonizeWorkerThreadData*)data;
	u8* dataPtr = data + sizeof(PolygonizeWorkerThreadData);

	TerrainVertexFixedPoint* fixedPointVerts = (TerrainVertexFixedPoint*)dataPtr;
	rVal->Vertices = (TerrainVertex*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainVertex);
	rVal->Indices = (u32*)dataPtr;
	dataPtr += TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32);
	rVal->VoxelData = (i8*)dataPtr;

	rVal->VerticesSize = TERRAIN_CELL_VERTEX_ARRAY_SIZE;
	rVal->IndicesSize = TERRAIN_CELL_INDEX_ARRAY_SIZE;
	rVal->Node = cellToPolygonize;
	rVal->OutputtedVertices = 0;
	rVal->OutputtedIndices = 0;
	rVal->MyAllocator = Allocator;

	std::function<i8(u8,u8,u8)> GetVoxelValueAt = [rVal](u8 x, u8 y, u8 z) -> i8
	{
		return rVal->VoxelData[TOTAL_DECK_SIZE * z + TOTAL_CELL_SIZE * y + x];
	};


	u32 verticesTop = 0;
	u32 indicesTop = 0;

	source->GetVoxelsForNode(cellToPolygonize, rVal->VoxelData);

	
	glm::ivec3 blockBottomLeft = cellToPolygonize->GetBottomLeftCorner();
	float cellSize = cellToPolygonize->GetSizeInVoxels();
	float stepSize = cellSize / BASE_CELL_SIZE;
	for (i32 z = 0; z < BASE_CELL_SIZE; z++)
	{
		for (i32 y=0; y < BASE_CELL_SIZE; y++)
		{
			for (i32 x = 0; x < BASE_CELL_SIZE; x++)
			{
				GridCell g;
				// maintain a history for vertex reuse
				

				// fill corner values - each corner of cell
				u8 caseIndex = 0;
				
				/*for (u8 cellZ = 0; cellZ < 2; cellZ++)
				{
					for (u8 cellY = 0; cellY < 2; cellY++)
					{
						for (u8 cellX = 0; cellX < 2; cellX++)
						{
							u8 cellI = cellZ * 4 + cellY * 2 + cellX;
							cell.val[cellI] = GetVoxelValueAt(x + cellX, y + cellY, z + cellZ);
							cell.p[cellI] = glm::vec3(
								blockBottomLeft.x + stepSize*x + stepSize*cellX,
								blockBottomLeft.y + stepSize*y + stepSize*cellY,
								blockBottomLeft.z + stepSize*z + stepSize*cellZ);
						}
					}
				}*/
				g.p[0] = glm::vec3(blockBottomLeft.x + stepSize * (x+0),blockBottomLeft.y + stepSize * (y+0),blockBottomLeft.z + stepSize * (z+0));
				g.p[1] = glm::vec3(blockBottomLeft.x + stepSize * (x+1),blockBottomLeft.y + stepSize * (y+0),blockBottomLeft.z + stepSize * (z+0));
				g.p[2] = glm::vec3(blockBottomLeft.x + stepSize * (x+1),blockBottomLeft.y + stepSize * (y+1),blockBottomLeft.z + stepSize * (z+0));
				g.p[3] = glm::vec3(blockBottomLeft.x + stepSize * (x+0),blockBottomLeft.y + stepSize * (y+1),blockBottomLeft.z + stepSize * (z+0));

				g.val[0] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x,POLYGONIZER_NEGATIVE_GUTTER + y, POLYGONIZER_NEGATIVE_GUTTER + z);
				g.val[1] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x + 1, POLYGONIZER_NEGATIVE_GUTTER + y, POLYGONIZER_NEGATIVE_GUTTER + z);
				g.val[2] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z);
				g.val[3] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z);

				g.coords[0] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x,POLYGONIZER_NEGATIVE_GUTTER + y, POLYGONIZER_NEGATIVE_GUTTER + z);
				g.coords[1] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x + 1, POLYGONIZER_NEGATIVE_GUTTER + y, POLYGONIZER_NEGATIVE_GUTTER + z);
				g.coords[2] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z);
				g.coords[3] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z);

				g.p[4] = glm::vec3(blockBottomLeft.x + stepSize * (x+0),blockBottomLeft.y + stepSize * (y+0),blockBottomLeft.z + stepSize * (z+1));
				g.p[5] = glm::vec3(blockBottomLeft.x + stepSize * (x+1),blockBottomLeft.y + stepSize * (y+0),blockBottomLeft.z + stepSize * (z+1));
				g.p[6] = glm::vec3(blockBottomLeft.x + stepSize * (x+1),blockBottomLeft.y + stepSize * (y+1),blockBottomLeft.z + stepSize * (z+1));
				g.p[7] = glm::vec3(blockBottomLeft.x + stepSize * (x+0),blockBottomLeft.y + stepSize * (y+1),blockBottomLeft.z + stepSize * (z+1));

				g.val[4] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER +y, POLYGONIZER_NEGATIVE_GUTTER +z+1);//BottomVals[x*_YCount+y];
				g.val[5] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER +y, POLYGONIZER_NEGATIVE_GUTTER +z+1);//BottomVals[(x+1)*_YCount+y];
				g.val[6] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z+1);//BottomVals[(x+1)*_YCount+y+1];
				g.val[7] = (float)GetVoxelValueAt(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z+1);//BottomVals[x*_YCount+y+1];

				g.coords[4] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER +y, POLYGONIZER_NEGATIVE_GUTTER +z+1);
				g.coords[5] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER +y, POLYGONIZER_NEGATIVE_GUTTER +z+1);
				g.coords[6] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x+1, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z+1);
				g.coords[7] = glm::ivec3(POLYGONIZER_NEGATIVE_GUTTER + x, POLYGONIZER_NEGATIVE_GUTTER + y+1, POLYGONIZER_NEGATIVE_GUTTER +z+1);

				TerrainVertex outputtedVerts[12];
				char outputtedIndices[36];

				int numOutputtedVerts = 0;
				int numOutputtedIndices = 0;

				Polygonise(g, numOutputtedVerts, outputtedVerts, numOutputtedIndices, outputtedIndices,rVal->VoxelData, cellToPolygonize, source);

				for (int i = 0; i < numOutputtedIndices; i++)
				{
					rVal->Indices[rVal->OutputtedIndices++] = rVal->OutputtedVertices + outputtedIndices[i];
				}

				for (int i = 0; i < numOutputtedVerts; i++)
				{
					rVal->Vertices[rVal->OutputtedVertices++] = outputtedVerts[i];
				}

				
			}
		}
	}
	
	return rVal;
}

typedef i8 Voxel;


inline Voxel GetVoxel(const Voxel *field, i32 n, i32 m, i32 i, i32 j, i32 k)
{
	return field[TOTAL_DECK_SIZE * (k+POLYGONIZER_NEGATIVE_GUTTER) + TOTAL_CELL_SIZE * (j+POLYGONIZER_NEGATIVE_GUTTER) + (i+POLYGONIZER_NEGATIVE_GUTTER)];
	//return (field[(k * m + j) * n + i]);
}

u32 LoadCell(const Voxel *field, i32 n, i32 m, i32 i, i32 j, i32 k, Voxel *distance)
{
	distance[0] = GetVoxel(field, n, m, i, j, k);
	distance[1] = GetVoxel(field, n, m, i + 1, j, k);
	distance[2] = GetVoxel(field, n, m, i, j + 1, k);
	distance[3] = GetVoxel(field, n, m, i + 1, j + 1, k);
	distance[4] = GetVoxel(field, n, m, i, j, k + 1);
	distance[5] = GetVoxel(field, n, m, i + 1, j, k + 1);
	distance[6] = GetVoxel(field, n, m, i, j + 1, k + 1);
	distance[7] = GetVoxel(field, n, m, i + 1, j + 1, k + 1);

	// Concatenate sign bits of the voxel values to form the case index for the cell.
	return (((distance[0] >> 7) & 0x01) | ((distance[1] >> 6) & 0x02)
	      | ((distance[2] >> 5) & 0x04) | ((distance[3] >> 4) & 0x08)
	      | ((distance[4] >> 3) & 0x10) | ((distance[5] >> 2) & 0x20)
	      | ((distance[6] >> 1) & 0x40) |  (distance[7] & 0x80));
}

// Listing 10.21


const u8 equivClassTable[256] =
{
	// Equivalence class index for each of the 256 possible cases.

	0x00, 0x01, 0x01, 0x03, 0x01, 0x03, 0x02, 0x04, 0x01, 0x02, 0x03, 0x04, 0x03, 0x04, 0x04, 0x03,
	0x01, 0x03, 0x02, 0x04, 0x02, 0x04, 0x06, 0x0C, 0x02, 0x05, 0x05, 0x0B, 0x05, 0x0A, 0x07, 0x04,
	0x01, 0x02, 0x03, 0x04, 0x02, 0x05, 0x05, 0x0A, 0x02, 0x06, 0x04, 0x0C, 0x05, 0x07, 0x0B, 0x04,
	0x03, 0x04, 0x04, 0x03, 0x05, 0x0B, 0x07, 0x04, 0x05, 0x07, 0x0A, 0x04, 0x08, 0x0E, 0x0E, 0x03,
	0x01, 0x02, 0x02, 0x05, 0x03, 0x04, 0x05, 0x0B, 0x02, 0x06, 0x05, 0x07, 0x04, 0x0C, 0x0A, 0x04,
	0x03, 0x04, 0x05, 0x0A, 0x04, 0x03, 0x07, 0x04, 0x05, 0x07, 0x08, 0x0E, 0x0B, 0x04, 0x0E, 0x03,
	0x02, 0x06, 0x05, 0x07, 0x05, 0x07, 0x08, 0x0E, 0x06, 0x09, 0x07, 0x0F, 0x07, 0x0F, 0x0E, 0x0D,
	0x04, 0x0C, 0x0B, 0x04, 0x0A, 0x04, 0x0E, 0x03, 0x07, 0x0F, 0x0E, 0x0D, 0x0E, 0x0D, 0x02, 0x01,
	0x01, 0x02, 0x02, 0x05, 0x02, 0x05, 0x06, 0x07, 0x03, 0x05, 0x04, 0x0A, 0x04, 0x0B, 0x0C, 0x04,
	0x02, 0x05, 0x06, 0x07, 0x06, 0x07, 0x09, 0x0F, 0x05, 0x08, 0x07, 0x0E, 0x07, 0x0E, 0x0F, 0x0D,
	0x03, 0x05, 0x04, 0x0B, 0x05, 0x08, 0x07, 0x0E, 0x04, 0x07, 0x03, 0x04, 0x0A, 0x0E, 0x04, 0x03,
	0x04, 0x0A, 0x0C, 0x04, 0x07, 0x0E, 0x0F, 0x0D, 0x0B, 0x0E, 0x04, 0x03, 0x0E, 0x02, 0x0D, 0x01,
	0x03, 0x05, 0x05, 0x08, 0x04, 0x0A, 0x07, 0x0E, 0x04, 0x07, 0x0B, 0x0E, 0x03, 0x04, 0x04, 0x03,
	0x04, 0x0B, 0x07, 0x0E, 0x0C, 0x04, 0x0F, 0x0D, 0x0A, 0x0E, 0x0E, 0x02, 0x04, 0x03, 0x0D, 0x01,
	0x04, 0x07, 0x0A, 0x0E, 0x0B, 0x0E, 0x0E, 0x02, 0x0C, 0x0F, 0x04, 0x0D, 0x04, 0x0D, 0x03, 0x01,
	0x03, 0x04, 0x04, 0x03, 0x04, 0x03, 0x0D, 0x01, 0x04, 0x0D, 0x03, 0x01, 0x03, 0x01, 0x01, 0x00
};

struct ClassData
{
	u8		geometryCounts;		// Vertex and triangle counts.
	u8		vertexIndex[15];	// List of 3-15 vertex indices.
};

const ClassData classGeometryTable[16] =
{
	// Triangulation data for each equivalence class.

	{0x00, {}},
	{0x31, {0, 1, 2}},
	{0x62, {0, 1, 2, 3, 4, 5}},
	{0x42, {0, 1, 2, 0, 2, 3}},
	{0x53, {0, 1, 4, 1, 3, 4, 1, 2, 3}},
	{0x73, {0, 1, 2, 0, 2, 3, 4, 5, 6}},
	{0x93, {0, 1, 2, 3, 4, 5, 6, 7, 8}},
	{0x84, {0, 1, 4, 1, 3, 4, 1, 2, 3, 5, 6, 7}},
	{0x84, {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7}},
	{0xC4, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
	{0x64, {0, 4, 5, 0, 1, 4, 1, 3, 4, 1, 2, 3}},
	{0x64, {0, 5, 4, 0, 4, 1, 1, 4, 3, 1, 3, 2}},
	{0x64, {0, 4, 5, 0, 3, 4, 0, 1, 3, 1, 2, 3}},
	{0x64, {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5}},
	{0x75, {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6}},
	{0x95, {0, 4, 5, 0, 3, 4, 0, 1, 3, 1, 2, 3, 6, 7, 8}}
};

const u16 vertexCodeTable[256][12] =
{
	// List of 3-12 vertex codes for each of the 256 possible cases.
	// The meanings of the bit fields are shown in Figure 10.33.

	{},
	{0xA188, 0x9050, 0x72E0},
	{0xA188, 0x65E9, 0x8359},
	{0x9050, 0x72E0, 0x65E9, 0x8359},
	{0x9050, 0x849A, 0x18F2},
	{0x72E0, 0xA188, 0x849A, 0x18F2},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x72E0, 0x65E9, 0x8359},
	{0x8359, 0x0BFB, 0x849A},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB},
	{0xA188, 0x65E9, 0x0BFB, 0x849A},
	{0x9050, 0x72E0, 0x65E9, 0x0BFB, 0x849A},
	{0x9050, 0x8359, 0x0BFB, 0x18F2},
	{0x8359, 0x0BFB, 0x18F2, 0x72E0, 0xA188},
	{0xA188, 0x65E9, 0x0BFB, 0x18F2, 0x9050},
	{0x72E0, 0x65E9, 0x0BFB, 0x18F2},
	{0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC},
	{0xA188, 0x65E9, 0x8359, 0x72E0, 0x1674, 0x27AC},
	{0x65E9, 0x8359, 0x9050, 0x1674, 0x27AC},
	{0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x1674, 0x27AC, 0xA188, 0x849A, 0x18F2},
	{0x72E0, 0x1674, 0x27AC, 0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x1674, 0x27AC, 0x65E9, 0x8359},
	{0x849A, 0x8359, 0x0BFB, 0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x849A, 0x8359, 0x0BFB},
	{0x849A, 0xA188, 0x65E9, 0x0BFB, 0x72E0, 0x1674, 0x27AC},
	{0x849A, 0x0BFB, 0x65E9, 0x27AC, 0x1674, 0x9050},
	{0x9050, 0x8359, 0x0BFB, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x8359, 0x0BFB, 0x18F2, 0x1674, 0x27AC, 0xA188},
	{0xA188, 0x65E9, 0x0BFB, 0x18F2, 0x9050, 0x72E0, 0x1674, 0x27AC},
	{0x27AC, 0x65E9, 0x0BFB, 0x18F2, 0x1674},
	{0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x9050, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0x8359, 0xA188, 0x27AC, 0x097D},
	{0x27AC, 0x097D, 0x8359, 0x9050, 0x72E0},
	{0x9050, 0x849A, 0x18F2, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x27AC, 0x097D, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x72E0, 0x27AC, 0x097D, 0x8359},
	{0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0x0BFB, 0x849A, 0xA188, 0x27AC, 0x097D},
	{0x9050, 0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x849A},
	{0x9050, 0x8359, 0x0BFB, 0x18F2, 0x65E9, 0x27AC, 0x097D},
	{0x8359, 0x0BFB, 0x18F2, 0x72E0, 0xA188, 0x65E9, 0x27AC, 0x097D},
	{0x9050, 0x18F2, 0x0BFB, 0x097D, 0x27AC, 0xA188},
	{0x097D, 0x0BFB, 0x18F2, 0x72E0, 0x27AC},
	{0x65E9, 0x72E0, 0x1674, 0x097D},
	{0xA188, 0x9050, 0x1674, 0x097D, 0x65E9},
	{0x72E0, 0x1674, 0x097D, 0x8359, 0xA188},
	{0x8359, 0x9050, 0x1674, 0x097D},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x9050, 0x849A, 0x18F2},
	{0x18F2, 0x849A, 0xA188, 0x65E9, 0x097D, 0x1674},
	{0x72E0, 0x1674, 0x097D, 0x8359, 0xA188, 0x9050, 0x849A, 0x18F2},
	{0x18F2, 0x1674, 0x097D, 0x8359, 0x849A},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x849A, 0x8359, 0x0BFB},
	{0xA188, 0x9050, 0x1674, 0x097D, 0x65E9, 0x849A, 0x8359, 0x0BFB},
	{0x72E0, 0x1674, 0x097D, 0x0BFB, 0x849A, 0xA188},
	{0x849A, 0x9050, 0x1674, 0x097D, 0x0BFB},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x9050, 0x8359, 0x0BFB, 0x18F2},
	{0xA188, 0x8359, 0x0BFB, 0x18F2, 0x1674, 0x097D, 0x65E9},
	{0xA188, 0x72E0, 0x1674, 0x097D, 0x0BFB, 0x18F2, 0x9050},
	{0x18F2, 0x1674, 0x097D, 0x0BFB},
	{0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x65E9, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x72E0, 0x65E9, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x849A, 0x0ABE, 0x1674},
	{0x72E0, 0xA188, 0x849A, 0x0ABE, 0x1674},
	{0x9050, 0x849A, 0x0ABE, 0x1674, 0xA188, 0x65E9, 0x8359},
	{0x1674, 0x0ABE, 0x849A, 0x8359, 0x65E9, 0x72E0},
	{0x8359, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x65E9, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x72E0, 0x65E9, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0x0ABE, 0x1674, 0x9050, 0x8359, 0x0BFB},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x1674, 0x72E0},
	{0xA188, 0x65E9, 0x0BFB, 0x0ABE, 0x1674, 0x9050},
	{0x1674, 0x72E0, 0x65E9, 0x0BFB, 0x0ABE},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC},
	{0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x9050},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x65E9, 0x8359},
	{0x18F2, 0x0ABE, 0x27AC, 0x65E9, 0x8359, 0x9050},
	{0x9050, 0x849A, 0x0ABE, 0x27AC, 0x72E0},
	{0xA188, 0x849A, 0x0ABE, 0x27AC},
	{0x9050, 0x849A, 0x0ABE, 0x27AC, 0x72E0, 0xA188, 0x65E9, 0x8359},
	{0x8359, 0x849A, 0x0ABE, 0x27AC, 0x65E9},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0x849A, 0x8359, 0x0BFB},
	{0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x9050, 0x849A, 0x8359, 0x0BFB},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0x849A, 0xA188, 0x65E9, 0x0BFB},
	{0x9050, 0x18F2, 0x0ABE, 0x27AC, 0x65E9, 0x0BFB, 0x849A},
	{0x72E0, 0x27AC, 0x0ABE, 0x0BFB, 0x8359, 0x9050},
	{0x0BFB, 0x0ABE, 0x27AC, 0xA188, 0x8359},
	{0x9050, 0xA188, 0x65E9, 0x0BFB, 0x0ABE, 0x27AC, 0x72E0},
	{0x65E9, 0x0BFB, 0x0ABE, 0x27AC},
	{0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x27AC, 0x097D, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x27AC, 0x097D, 0x8359, 0x9050, 0x72E0, 0x18F2, 0x0ABE, 0x1674},
	{0x849A, 0x0ABE, 0x1674, 0x9050, 0x65E9, 0x27AC, 0x097D},
	{0x72E0, 0xA188, 0x849A, 0x0ABE, 0x1674, 0x65E9, 0x27AC, 0x097D},
	{0x849A, 0x0ABE, 0x1674, 0x9050, 0xA188, 0x27AC, 0x097D, 0x8359},
	{0x72E0, 0x27AC, 0x097D, 0x8359, 0x849A, 0x0ABE, 0x1674},
	{0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0x0BFB, 0x849A, 0xA188, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0x849A, 0x9050, 0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x18F2, 0x0ABE, 0x1674},
	{0x0ABE, 0x1674, 0x9050, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x1674, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0x0BFB, 0x0ABE, 0x1674, 0x9050, 0xA188, 0x27AC, 0x097D},
	{0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x0ABE, 0x1674},
	{0x097D, 0x65E9, 0x72E0, 0x18F2, 0x0ABE},
	{0x0ABE, 0x097D, 0x65E9, 0xA188, 0x9050, 0x18F2},
	{0x0ABE, 0x18F2, 0x72E0, 0xA188, 0x8359, 0x097D},
	{0x0ABE, 0x097D, 0x8359, 0x9050, 0x18F2},
	{0x9050, 0x849A, 0x0ABE, 0x097D, 0x65E9, 0x72E0},
	{0x65E9, 0xA188, 0x849A, 0x0ABE, 0x097D},
	{0x72E0, 0x9050, 0x849A, 0x0ABE, 0x097D, 0x8359, 0xA188},
	{0x8359, 0x849A, 0x0ABE, 0x097D},
	{0x097D, 0x65E9, 0x72E0, 0x18F2, 0x0ABE, 0x849A, 0x8359, 0x0BFB},
	{0x097D, 0x65E9, 0xA188, 0x9050, 0x18F2, 0x0ABE, 0x849A, 0x8359, 0x0BFB},
	{0x097D, 0x0BFB, 0x849A, 0xA188, 0x72E0, 0x18F2, 0x0ABE},
	{0x9050, 0x18F2, 0x0ABE, 0x097D, 0x0BFB, 0x849A},
	{0x0ABE, 0x097D, 0x65E9, 0x72E0, 0x9050, 0x8359, 0x0BFB},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x097D, 0x65E9},
	{0xA188, 0x72E0, 0x9050, 0x0BFB, 0x0ABE, 0x097D},
	{0x0BFB, 0x0ABE, 0x097D},
	{0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x9050, 0x72E0, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x72E0, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0x849A, 0x18F2, 0x72E0, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x8359, 0x097D, 0x0ABE, 0x849A},
	{0x849A, 0x8359, 0x097D, 0x0ABE, 0xA188, 0x9050, 0x72E0},
	{0x097D, 0x0ABE, 0x849A, 0xA188, 0x65E9},
	{0x72E0, 0x65E9, 0x097D, 0x0ABE, 0x849A, 0x9050},
	{0x18F2, 0x9050, 0x8359, 0x097D, 0x0ABE},
	{0x097D, 0x8359, 0xA188, 0x72E0, 0x18F2, 0x0ABE},
	{0x18F2, 0x9050, 0xA188, 0x65E9, 0x097D, 0x0ABE},
	{0x0ABE, 0x18F2, 0x72E0, 0x65E9, 0x097D},
	{0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x65E9, 0x8359, 0x9050, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x1674, 0x27AC, 0xA188, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x8359, 0x849A, 0x18F2, 0x1674, 0x27AC, 0x65E9, 0x0BFB, 0x097D, 0x0ABE},
	{0x849A, 0x8359, 0x097D, 0x0ABE, 0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x849A, 0x8359, 0x097D, 0x0ABE},
	{0x097D, 0x0ABE, 0x849A, 0xA188, 0x65E9, 0x72E0, 0x1674, 0x27AC},
	{0x65E9, 0x097D, 0x0ABE, 0x849A, 0x9050, 0x1674, 0x27AC},
	{0x18F2, 0x9050, 0x8359, 0x097D, 0x0ABE, 0x72E0, 0x1674, 0x27AC},
	{0x18F2, 0x1674, 0x27AC, 0xA188, 0x8359, 0x097D, 0x0ABE},
	{0x9050, 0xA188, 0x65E9, 0x097D, 0x0ABE, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x18F2, 0x1674, 0x27AC, 0x65E9, 0x097D, 0x0ABE},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB, 0xA188, 0x9050, 0x72E0},
	{0x8359, 0xA188, 0x27AC, 0x0ABE, 0x0BFB},
	{0x9050, 0x8359, 0x0BFB, 0x0ABE, 0x27AC, 0x72E0},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB, 0x9050, 0x849A, 0x18F2},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x0BFB, 0x65E9, 0x27AC, 0x0ABE},
	{0x8359, 0xA188, 0x27AC, 0x0ABE, 0x0BFB, 0x9050, 0x849A, 0x18F2},
	{0x8359, 0x849A, 0x18F2, 0x72E0, 0x27AC, 0x0ABE, 0x0BFB},
	{0x65E9, 0x27AC, 0x0ABE, 0x849A, 0x8359},
	{0x65E9, 0x27AC, 0x0ABE, 0x849A, 0x8359, 0xA188, 0x9050, 0x72E0},
	{0xA188, 0x27AC, 0x0ABE, 0x849A},
	{0x72E0, 0x27AC, 0x0ABE, 0x849A, 0x9050},
	{0x9050, 0x8359, 0x65E9, 0x27AC, 0x0ABE, 0x18F2},
	{0x8359, 0x65E9, 0x27AC, 0x0ABE, 0x18F2, 0x72E0, 0xA188},
	{0x9050, 0xA188, 0x27AC, 0x0ABE, 0x18F2},
	{0x72E0, 0x27AC, 0x0ABE, 0x18F2},
	{0x0ABE, 0x0BFB, 0x65E9, 0x72E0, 0x1674},
	{0x9050, 0x1674, 0x0ABE, 0x0BFB, 0x65E9, 0xA188},
	{0x72E0, 0x1674, 0x0ABE, 0x0BFB, 0x8359, 0xA188},
	{0x0BFB, 0x8359, 0x9050, 0x1674, 0x0ABE},
	{0x0ABE, 0x0BFB, 0x65E9, 0x72E0, 0x1674, 0x9050, 0x849A, 0x18F2},
	{0x1674, 0x0ABE, 0x0BFB, 0x65E9, 0xA188, 0x849A, 0x18F2},
	{0x0ABE, 0x0BFB, 0x8359, 0xA188, 0x72E0, 0x1674, 0x9050, 0x849A, 0x18F2},
	{0x8359, 0x849A, 0x18F2, 0x1674, 0x0ABE, 0x0BFB},
	{0x72E0, 0x65E9, 0x8359, 0x849A, 0x0ABE, 0x1674},
	{0x65E9, 0xA188, 0x9050, 0x1674, 0x0ABE, 0x849A, 0x8359},
	{0x1674, 0x0ABE, 0x849A, 0xA188, 0x72E0},
	{0x9050, 0x1674, 0x0ABE, 0x849A},
	{0x0ABE, 0x18F2, 0x9050, 0x8359, 0x65E9, 0x72E0, 0x1674},
	{0xA188, 0x8359, 0x65E9, 0x18F2, 0x1674, 0x0ABE},
	{0xA188, 0x72E0, 0x1674, 0x0ABE, 0x18F2, 0x9050},
	{0x18F2, 0x1674, 0x0ABE},
	{0x18F2, 0x0BFB, 0x097D, 0x1674},
	{0x0BFB, 0x097D, 0x1674, 0x18F2, 0xA188, 0x9050, 0x72E0},
	{0x0BFB, 0x097D, 0x1674, 0x18F2, 0xA188, 0x65E9, 0x8359},
	{0x8359, 0x9050, 0x72E0, 0x65E9, 0x18F2, 0x0BFB, 0x097D, 0x1674},
	{0x0BFB, 0x097D, 0x1674, 0x9050, 0x849A},
	{0xA188, 0x849A, 0x0BFB, 0x097D, 0x1674, 0x72E0},
	{0x0BFB, 0x097D, 0x1674, 0x9050, 0x849A, 0xA188, 0x65E9, 0x8359},
	{0x849A, 0x0BFB, 0x097D, 0x1674, 0x72E0, 0x65E9, 0x8359},
	{0x849A, 0x8359, 0x097D, 0x1674, 0x18F2},
	{0x849A, 0x8359, 0x097D, 0x1674, 0x18F2, 0xA188, 0x9050, 0x72E0},
	{0x1674, 0x097D, 0x65E9, 0xA188, 0x849A, 0x18F2},
	{0x849A, 0x9050, 0x72E0, 0x65E9, 0x097D, 0x1674, 0x18F2},
	{0x8359, 0x097D, 0x1674, 0x9050},
	{0xA188, 0x8359, 0x097D, 0x1674, 0x72E0},
	{0x65E9, 0x097D, 0x1674, 0x9050, 0xA188},
	{0x65E9, 0x097D, 0x1674, 0x72E0},
	{0x27AC, 0x72E0, 0x18F2, 0x0BFB, 0x097D},
	{0xA188, 0x27AC, 0x097D, 0x0BFB, 0x18F2, 0x9050},
	{0x27AC, 0x72E0, 0x18F2, 0x0BFB, 0x097D, 0xA188, 0x65E9, 0x8359},
	{0x27AC, 0x65E9, 0x8359, 0x9050, 0x18F2, 0x0BFB, 0x097D},
	{0x849A, 0x0BFB, 0x097D, 0x27AC, 0x72E0, 0x9050},
	{0x097D, 0x27AC, 0xA188, 0x849A, 0x0BFB},
	{0x27AC, 0x72E0, 0x9050, 0x849A, 0x0BFB, 0x097D, 0x8359, 0xA188, 0x65E9},
	{0x849A, 0x0BFB, 0x097D, 0x27AC, 0x65E9, 0x8359},
	{0x8359, 0x097D, 0x27AC, 0x72E0, 0x18F2, 0x849A},
	{0x18F2, 0x849A, 0x8359, 0x097D, 0x27AC, 0xA188, 0x9050},
	{0x097D, 0x27AC, 0x72E0, 0x18F2, 0x849A, 0xA188, 0x65E9},
	{0x9050, 0x18F2, 0x849A, 0x65E9, 0x097D, 0x27AC},
	{0x72E0, 0x9050, 0x8359, 0x097D, 0x27AC},
	{0x8359, 0x097D, 0x27AC, 0xA188},
	{0x9050, 0xA188, 0x65E9, 0x097D, 0x27AC, 0x72E0},
	{0x65E9, 0x097D, 0x27AC},
	{0x1674, 0x18F2, 0x0BFB, 0x65E9, 0x27AC},
	{0x1674, 0x18F2, 0x0BFB, 0x65E9, 0x27AC, 0xA188, 0x9050, 0x72E0},
	{0xA188, 0x27AC, 0x1674, 0x18F2, 0x0BFB, 0x8359},
	{0x27AC, 0x1674, 0x18F2, 0x0BFB, 0x8359, 0x9050, 0x72E0},
	{0x9050, 0x1674, 0x27AC, 0x65E9, 0x0BFB, 0x849A},
	{0x1674, 0x72E0, 0xA188, 0x849A, 0x0BFB, 0x65E9, 0x27AC},
	{0x0BFB, 0x8359, 0xA188, 0x27AC, 0x1674, 0x9050, 0x849A},
	{0x849A, 0x0BFB, 0x8359, 0x72E0, 0x27AC, 0x1674},
	{0x8359, 0x65E9, 0x27AC, 0x1674, 0x18F2, 0x849A},
	{0x1674, 0x18F2, 0x849A, 0x8359, 0x65E9, 0x27AC, 0xA188, 0x9050, 0x72E0},
	{0x18F2, 0x849A, 0xA188, 0x27AC, 0x1674},
	{0x849A, 0x9050, 0x72E0, 0x27AC, 0x1674, 0x18F2},
	{0x27AC, 0x1674, 0x9050, 0x8359, 0x65E9},
	{0x8359, 0x65E9, 0x27AC, 0x1674, 0x72E0, 0xA188},
	{0xA188, 0x27AC, 0x1674, 0x9050},
	{0x72E0, 0x27AC, 0x1674},
	{0x72E0, 0x18F2, 0x0BFB, 0x65E9},
	{0x9050, 0x18F2, 0x0BFB, 0x65E9, 0xA188},
	{0xA188, 0x72E0, 0x18F2, 0x0BFB, 0x8359},
	{0x9050, 0x18F2, 0x0BFB, 0x8359},
	{0x849A, 0x0BFB, 0x65E9, 0x72E0, 0x9050},
	{0xA188, 0x849A, 0x0BFB, 0x65E9},
	{0x72E0, 0x9050, 0x849A, 0x0BFB, 0x8359, 0xA188},
	{0x8359, 0x849A, 0x0BFB},
	{0x8359, 0x65E9, 0x72E0, 0x18F2, 0x849A},
	{0x18F2, 0x849A, 0x8359, 0x65E9, 0xA188, 0x9050},
	{0x72E0, 0x18F2, 0x849A, 0xA188},
	{0x9050, 0x18F2, 0x849A},
	{0x9050, 0x8359, 0x65E9, 0x72E0},
	{0xA188, 0x8359, 0x65E9},
	{0xA188, 0x72E0, 0x9050},
	{}
};

// Listing 10.22

typedef glm::ivec3 Integer3D;

struct CellStorage
{
	u16		corner[7];
	u16		edge[9];
};

struct Triangle
{
	u32		vertexIndex[3];
};

inline u16 *ReuseCornerVertex(i32 n, i32 i, i32 j, CellStorage *const (& deckStorage)[2], u16 cornerIndex, u16 deltaCode)
{
	// The corner index in the preceding cell is the sum of the original
	// corner index and the masked delta code.
	cornerIndex += deltaCode;

	// The three bits of the delta code indicate whether one should
	// be subtracted from the cell coords in the x, y, and z directions.
	i32 dx = deltaCode & 1;
	i32 dy = (deltaCode >> 1) & 1;
	i32 dz = deltaCode >> 2;

	// deckStorage[0] points to the current deck, and
	// deckStorage[1] points to the preceding deck
	CellStorage *deck = deckStorage[dz];

	// Return the address of the vertex index in the preceding cell.
	// The new corner index can never be zero.
	return (&deck[(j - dy) * n + (i - dx)].corner[cornerIndex - 1]);
}

inline u16 ReuseEdgeVertex(i32 n, i32 i, i32 j, CellStorage *const (& deckStorage)[2], u16 edgeIndex, u16 deltaCode)
{
	// Edge index in preceding cell differs from original edge index by 3 for each of
	// the lowest three bits in the masked delta code, and by 6 for the highest bit.
	edgeIndex += ((deltaCode & 1) + ((deltaCode >> 1) & 1) + ((deltaCode >> 2) & 3)) * 3;

	// Bits 0, 1, and 3 of the delta code indicate whether one should
	// be subtracted from the cell coords in the x, y, and z directions.
	i32 dx = deltaCode & 1;
	i32 dy = (deltaCode >> 1) & 1;
	i32 dz = deltaCode >> 3;

	// deckStorage[0] points to the current deck, and
	// deckStorage[1] points to the preceding deck
	const CellStorage *deck = deckStorage[dz];

	// Return the vertex index stored in the preceding cell.
	// The new edge index can never be less than 3.
	return (deck[(j - dy) * n + (i - dx)].edge[edgeIndex - 3]);
}

// Listing 10.23
//#pragma optimize("", off)


//#pragma optimize("", off)
typedef signed long fix;
#define FIXED_POINT_FRACTIONAL_BITS 8
#define multfix(a,b) (fix)(((((signed long long) a))*(((signed long long) b)) >> 8))

#define divfix(a,b) ((fix)((( signed long long)(a) / (b) << 8)))

#define int2fix(a) (fix)((a) << 8)
#define fix2int(a) ((int))((a >> 8))
#define fix2float(input) ((float)input / (float)(1 << FIXED_POINT_FRACTIONAL_BITS))
#define float2fix(input) ((fix)(round(input * (1 << FIXED_POINT_FRACTIONAL_BITS))))
#define sqrtfix(a) (float2fix(sqrt(fix2float(a))))     // apparently this is the fastest (in terms of runtime) way to do a square root on a fixed point number

Integer3D NormalizeFixedPointVector(const Integer3D& inVec)
{
	/* I am not sure this function is actually doing what it should be */
	fix x2 = multfix(inVec.x, inVec.x);
	fix y2 = multfix(inVec.y, inVec.y);
	fix z2 = multfix(inVec.z, inVec.z);
	fix length = sqrtfix((x2 + y2 + z2));

	Integer3D val =  
	{
		length ? divfix(x2,length) : 0,
		length ? divfix(y2,length) : 0,
		length ? divfix(z2,length) : 0
	};

	glm::vec3 fp = 
		{
			fix2float(val.x),
			fix2float(val.y),
			fix2float(val.z)
		};
	return val;
}

//#pragma optimize("", on)

void SurfaceShift(u8 currentLOD, Integer3D& minSample, Integer3D& maxSample, IVoxelDataSource* source, i32& d0, i32& d1)
{
	if (!currentLOD)
	{
		return;
	}
	Integer3D midSample = minSample + (minSample + maxSample) / 2;
	Voxel sample = source->GetVoxelAt(midSample);
	if (d0 < 0)
	{
		if (sample < 0)
		{
			minSample = midSample;
			d0 = sample;
		}
		else
		{
			maxSample = midSample;
			d1 = sample;
		}
	}
	else
	{
		if (sample < 0)
		{
			maxSample = midSample;
			d1 = sample;
		}
		else
		{
			minSample = midSample;
			d0 = sample;
		}
	}
	SurfaceShift(currentLOD -1, minSample, maxSample, source, d0, d1);
}

void ProcessCell(const Voxel *field, i32 n, i32 m, i32 i, i32 j, i32 k, CellStorage *const (& deckStorage)[2], u32 deltaMask, i32& meshVertexCount, i32& meshTriangleCount, TerrainVertexFixedPoint *meshVertexArray, Triangle *meshTriangleArray, IVoxelDataSource* source, u8 lod)
{
	Voxel		distance[8];

	// Get storage for current cell and set vertex indices at corners
	// to invalid values so those not generated here won't get reused.
	CellStorage *cellStorage = &deckStorage[0][j * n + i];
	for (i32 a = 0; a < 7; a++) cellStorage->corner[a] = 0xFFFF;

	// Call LoadCell() to populate the distance array and get case index.
	u32 caseIndex = LoadCell(field, n, m, i, j, k, distance);

	// Look up the equivalence class index and use it to look up
	// geometric data for this cell. No geometry if case is 0 or 255.
	i32 equivClass = equivClassTable[caseIndex];
	const ClassData *classData = &classGeometryTable[equivClass];
	u32 geometryCounts = classData->geometryCounts;

	if (geometryCounts != 0)
	{
		u16		cellVertexIndex[12];

		i32 vertexCount = geometryCounts >> 4;
		i32 triangleCount = geometryCounts & 0x0F;

		// Look up vertex codes using original case index.
		const u16 *vertexCode = vertexCodeTable[caseIndex];

		// Duplicate middle bit of delta mask to construct 4-bit mask used for edges.
		u16 edgeDeltaMask = ((deltaMask << 1) & 0x0C) | (deltaMask & 0x03);

		for (i32 a = 0; a < vertexCount; a++)
		{
			u16			vertexIndex;
			u8			corner[2];
			Integer3D		position[2];
			Integer3D normal[2];

			// Extract corner numbers from low 6 bits of vertex code.
			u16 vcode = vertexCode[a];
			corner[0] = vcode & 0x07;
			corner[1] = (vcode >> 3) & 0x07;

			// Construct integer coordinates of edge's endpoints.
			position[0].x = i + (corner[0] & 1);
			position[0].y = j + ((corner[0] >> 1) & 1);
			position[0].z = k + ((corner[0] >> 2) & 1);
			position[1].x = i + (corner[1] & 1);
			position[1].y = j + ((corner[1] >> 1) & 1);
			position[1].z = k + ((corner[1] >> 2) & 1);

			normal[0].x = int2fix(GetVoxel(field, n, m, position[0].x-1, position[0].y, position[0].z) - GetVoxel(field, n, m, position[0].x+1, position[0].y, position[0].z));
			normal[0].y = int2fix(GetVoxel(field, n, m, position[0].x, position[0].y-1, position[0].z) - GetVoxel(field, n, m, position[0].x, position[0].y+1, position[0].z));
			normal[0].z = int2fix(GetVoxel(field, n, m, position[0].x, position[0].y, position[0].z-1) - GetVoxel(field, n, m, position[0].x, position[0].y, position[0].z+1));

			normal[1].x = int2fix(GetVoxel(field, n, m, position[1].x-1, position[1].y, position[1].z) - GetVoxel(field, n, m, position[1].x+1, position[1].y, position[1].z));
			normal[1].y = int2fix(GetVoxel(field, n, m, position[1].x, position[1].y-1, position[1].z) - GetVoxel(field, n, m, position[1].x, position[1].y+1, position[1].z));
			normal[1].z = int2fix(GetVoxel(field, n, m, position[1].x, position[1].y, position[1].z-1) - GetVoxel(field, n, m, position[1].x, position[1].y, position[1].z+1));

			normal[0] = NormalizeFixedPointVector(normal[0]);
			normal[1] = NormalizeFixedPointVector(normal[1]);


			// Calculate interpolation parameter with Equation (10.95).
			i32 d0 = distance[corner[0]];
			i32 d1 = distance[corner[1]];

			/*if (glm::length(glm::vec3(position[0])) > glm::length(glm::vec3(position[1])))
			{
				SurfaceShift(lod, position[1], position[0], source, d0, d1);
			}
			else
			{
				SurfaceShift(lod, position[0], position[1], source, d0, d1);

			}*/

			i32 t = (d1 << 8) / (d1 - d0);

			if ((t & 0x00FF) != 0)
			{
				// Vertex falls in the interior of an edge.
				// Extract edge index and delta code from vertex code.
				u16 edgeIndex = (vcode >> 8) & 0x0F;
				u16 deltaCode = (vcode >> 12) & edgeDeltaMask;

				if (deltaCode != 0)
				{
					// Reuse vertex from edge in preceding cell.
					vertexIndex = ReuseEdgeVertex(n, i, j, deckStorage, edgeIndex, deltaCode);
				}
				else
				{
					// Generate a new vertex with Equation (10.96).
					vertexIndex = meshVertexCount++;
					TerrainVertexFixedPoint *vertex = &meshVertexArray[vertexIndex];
					vertex->Position = position[0] * t + position[1] * (0x0100 - t);
					vertex->Normal = normal[0] * t + normal[1] * (0x0100 - t);

					if (edgeIndex >= 3)
					{
						// Store vertex index for potential reuse later.
						cellStorage->edge[edgeIndex - 3] = vertexIndex;
					}
				}

				cellVertexIndex[a] = vertexIndex;
			}
			else
			{
				// Vertex falls exactly at the first corner of the cell if
				// t == 0, and at the second corner if t == 0x0100.
				u8 c = (t == 0);
				u8 cornerIndex = corner[c];

				// Corner vertex in preceding cell may not have been
				// generated, so we get address and look for valid index.
				u16 *indexAddress = nullptr;
				u16 deltaCode = (cornerIndex ^ 7) & deltaMask;
				if (deltaCode != 0)
				{
					// Reuse vertex from corner in preceding cell.
					indexAddress = ReuseCornerVertex(n, i, j, deckStorage, cornerIndex, deltaCode);
				}
				else if (cornerIndex != 0)
				{
					// Vertex will be stored for potential reuse later.
					indexAddress = &cellStorage->corner[cornerIndex - 1];
				}

				vertexIndex = (indexAddress) ? *indexAddress : 0xFFFF;
				if (vertexIndex == 0xFFFF)
				{
					// Vertex was not previously generated.
					vertexIndex = meshVertexCount++;
					if (indexAddress) *indexAddress = vertexIndex;

					// Shift corner position to add 8 bits of fraction.
					meshVertexArray[vertexIndex].Position = position[c] << 8;
					meshVertexArray[vertexIndex].Normal = normal[c];
				}

				cellVertexIndex[a] = vertexIndex;
			}
		}

		// Generate triangles for this cell using table data.
		const u8 *classVertexIndex = classData->vertexIndex;
		Triangle *meshTriangle = &meshTriangleArray[meshTriangleCount];
		meshTriangleCount += triangleCount;

		for (i32 a = 0; a < triangleCount; a++)
		{
			meshTriangle[a].vertexIndex[0] = cellVertexIndex[classVertexIndex[0]];
			meshTriangle[a].vertexIndex[1] = cellVertexIndex[classVertexIndex[1]];
			meshTriangle[a].vertexIndex[2] = cellVertexIndex[classVertexIndex[2]];
			classVertexIndex += 3;
		}
	}
}

// Listing 10.24

void ExtractIsosurface(const Voxel *field, i32 n, i32 m, i32 h, i32 *meshVertexCount, i32 *meshTriangleCount, TerrainVertexFixedPoint *meshVertexArray, Triangle *meshTriangleArray, IAllocator* allocator, IVoxelDataSource* source, u8 lod)
{
	CellStorage* deckStorage[2];
	// Allocate storage for two decks of history.

	CellStorage* precedingCellStorage = IAllocator::NewArray<CellStorage>(allocator, BASE_CELL_SIZE * BASE_CELL_SIZE * 2);

	i32 vertexCount = 0;
	i32 triangleCount = 0;
	u16 deltaMask = 0;

	for (i32 k = 0; k < h; k++)
	{
		// Ping-pong between history decks.
		deckStorage[0] = &precedingCellStorage[n * m * (k & 1)];

		for (i32 j = 0; j < m; j++)
		{
			for (i32 i = 0; i < n; i++)
			{
				ProcessCell(field, n, m, i, j, k, deckStorage, deltaMask, vertexCount, triangleCount, meshVertexArray, meshTriangleArray, source, lod);
				deltaMask |= 1;  					// Allow reuse in x direction.
			}

			deltaMask = (deltaMask | 2) & 6;		// Allow reuse in y direction, but not x.
		}

		deckStorage[1] = deckStorage[0];			// Current deck becomes preceding deck.
		deltaMask = 4;								// Allow reuse only in z direction.
	}

	allocator->Free(precedingCellStorage);

	*meshVertexCount = vertexCount;
	*meshTriangleCount = triangleCount;
}

u32 LoadTransitionCellX(i8* field, IVoxelDataSource* source, i32 i, i32 j, i32 k, i8* outDistance, ITerrainOctreeNode* cellToPolygonize, u32 stepSize, const glm::ivec3& bl)
{
	glm::ivec3 ijk = {i,j,k};
	outDistance[0] = outDistance[0x9] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j, k);
	outDistance[1] = source->GetVoxelAt(ijk + glm::ivec3{0, (stepSize >> 1), 0} + bl);
	outDistance[2] = outDistance[0xa] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j + 1, k);
	outDistance[3] = source->GetVoxelAt(ijk + glm::ivec3{0, 0, (stepSize >> 1)} + bl);
	outDistance[4] = source->GetVoxelAt(ijk + glm::ivec3{0, (stepSize >> 1), (stepSize >> 1)} + bl);
	outDistance[5] = source->GetVoxelAt(ijk + glm::ivec3{0, stepSize, (stepSize >> 1)} + bl);
	outDistance[6] = outDistance[0xb] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j, k + 1);
	outDistance[7] = source->GetVoxelAt(ijk + glm::ivec3{0, (stepSize >> 1), stepSize} + bl);
	outDistance[8] = outDistance[0xc] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j + 1, k + 1);
	return 0
		| ((std::signbit(static_cast<float>(outDistance[0])) << 0) & 0x01)
		| ((std::signbit(static_cast<float>(outDistance[1])) << 1) & 0x02)
		| ((std::signbit(static_cast<float>(outDistance[2])) << 2) & 0x02)
		| ((std::signbit(static_cast<float>(outDistance[3])) << 7) & 0x80)
		| ((std::signbit(static_cast<float>(outDistance[4])) << 8) & 0x100)
		| ((std::signbit(static_cast<float>(outDistance[5])) << 3) & 0x08)
		| ((std::signbit(static_cast<float>(outDistance[6])) << 6) & 0x40)
		| ((std::signbit(static_cast<float>(outDistance[7])) << 5) & 0x20)
		| ((std::signbit(static_cast<float>(outDistance[8])) << 4) & 0x10);

}

u32 LoadTransitionCellY(i8* field, IVoxelDataSource* source, i32 i, i32 j, i32 k, i8* outDistance, ITerrainOctreeNode* cellToPolygonize, u32 stepSize, const glm::ivec3& bl)
{
	glm::ivec3 ijk = {i,j,k};
	outDistance[0] = outDistance[0x9] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j, k);
	outDistance[1] = source->GetVoxelAt(ijk + glm::ivec3{ (stepSize >> 1), 0, 0} + bl);
	outDistance[2] = outDistance[0xa] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i + 1, j, k);
	outDistance[3] = source->GetVoxelAt(ijk + glm::ivec3{0, 0, (stepSize >> 1)} + bl);
	outDistance[4] = source->GetVoxelAt(ijk + glm::ivec3{(stepSize >> 1), 0, (stepSize >> 1)} + bl);
	outDistance[5] = source->GetVoxelAt(ijk + glm::ivec3{stepSize, 0, (stepSize >> 1)} + bl);
	outDistance[6] = outDistance[0xb] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i, j, k + 1);
	outDistance[7] = source->GetVoxelAt(ijk + glm::ivec3{(stepSize >> 1), 0, stepSize} + bl);
	outDistance[8] = outDistance[0xc] = GetVoxel(field, BASE_CELL_SIZE, BASE_CELL_SIZE, i + 1, j, k + 1);
	return 0
		| ((std::signbit(static_cast<float>(outDistance[0])) << 0) & 0x01)
		| ((std::signbit(static_cast<float>(outDistance[1])) << 1) & 0x02)
		| ((std::signbit(static_cast<float>(outDistance[2])) << 2) & 0x02)
		| ((std::signbit(static_cast<float>(outDistance[3])) << 7) & 0x80)
		| ((std::signbit(static_cast<float>(outDistance[4])) << 8) & 0x100)
		| ((std::signbit(static_cast<float>(outDistance[5])) << 3) & 0x08)
		| ((std::signbit(static_cast<float>(outDistance[6])) << 6) & 0x40)
		| ((std::signbit(static_cast<float>(outDistance[7])) << 5) & 0x20)
		| ((std::signbit(static_cast<float>(outDistance[8])) << 4) & 0x10);

}


PolygonizeWorkerThreadData* TerrainPolygonizer::PolygonizeCellSyncMMC(ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source)
{
	size_t size = sizeof(Triangle);
	u32 cellOutputTop = 0;

	u8* data = (u8*)Allocator->Malloc(
		sizeof(PolygonizeWorkerThreadData) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainVertex) +
		TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32) +
		TOTAL_CELL_VOLUME_SIZE * sizeof(i8) +
		6 * (TERRAIN_CELL_TRANSITION_MESH_VERTEX_ARRAY_SIZE * sizeof(TerrainVertex)) + 
		6 * (TERRAIN_CELL_TRANSITION_MESH_INDEX_ARRAY_SIZE * sizeof(u32))
	); // malloc everything in a single block

	PolygonizeWorkerThreadData* rVal = (PolygonizeWorkerThreadData*)data;
	u8* dataPtr = data + sizeof(PolygonizeWorkerThreadData);

	TerrainVertexFixedPoint* fixedPointVerts = (TerrainVertexFixedPoint*)dataPtr;
	rVal->Vertices = (TerrainVertex*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainVertex);
	rVal->Tris = (Triangle*)dataPtr;
	dataPtr += TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32);
	rVal->VoxelData = (i8*)dataPtr;

	rVal->VerticesSize = TERRAIN_CELL_VERTEX_ARRAY_SIZE;
	rVal->IndicesSize = TERRAIN_CELL_INDEX_ARRAY_SIZE;
	rVal->Node = cellToPolygonize;
	rVal->OutputtedVertices = 0;
	rVal->OutputtedIndices = 0;
	rVal->MyAllocator = Allocator;


	CellStorage* deckStorage[2];


	u32 verticesTop = 0;
	u32 indicesTop = 0;

	source->GetVoxelsForNode(cellToPolygonize, rVal->VoxelData);

	ExtractIsosurface(rVal->VoxelData, 
		BASE_CELL_SIZE,
		BASE_CELL_SIZE,
		BASE_CELL_SIZE,
		(i32*)&rVal->OutputtedVertices,
		(i32*)&rVal->OutputtedIndices,
		fixedPointVerts,
		rVal->Tris,
		Allocator,
		source,
		cellToPolygonize->GetMipLevel());

	rVal->OutputtedIndices *= 3;

	glm::ivec3 blockBottomLeft = cellToPolygonize->GetBottomLeftCorner();
	float cellSize = cellToPolygonize->GetSizeInVoxels();
	float stepSize = cellSize / BASE_CELL_SIZE;

	// convert from fixed point to floating point, transforming to actual position and scale
	for (int i = 0; i < rVal->OutputtedVertices; i++)
	{
		TerrainVertexFixedPoint& fixed = fixedPointVerts[i];

		rVal->Vertices[i].Position = {
			fix2float(fixed.Position.x),
			fix2float(fixed.Position.y),
			fix2float(fixed.Position.z)
		};
		rVal->Vertices[i].Position *= stepSize;
		rVal->Vertices[i].Position += blockBottomLeft;
		rVal->Vertices[i].Normal = 
		glm::normalize(glm::vec3{
			fix2float(fixed.Normal.x),
			fix2float(fixed.Normal.y),
			fix2float(fixed.Normal.z)
		});
	}
	
	
	return rVal;
}
//#pragma optimize("", on)
