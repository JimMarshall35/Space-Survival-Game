#include "TerrainPolygonizer.h"
#include "IAllocator.h"
#include "TerrainDefs.h"
#include "IVoxelDataSource.h"
#include "TransVoxel.h"

#define TERRAIN_CELL_VERTEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum
#define TERRAIN_CELL_INDEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum

TerrainPolygonizer::TerrainPolygonizer(IAllocator* allocator)
	:Allocator(allocator),
	ThreadPool(std::thread::hardware_concurrency())
{
	size_t threadPoolSize = std::thread::hardware_concurrency();
}

TerrainPolygonizer::~TerrainPolygonizer()
{
	
}

std::future<PolygonizeWorkerThreadData*> TerrainPolygonizer::PolygonizeNodeAsync(ITerrainOctreeNode* node, IVoxelDataSource* source)
{
	//auto r = ThreadPool.enqueue(PolygonizeCellSync, node);
	auto r = ThreadPool.enqueue([this, node, source]() {
		return PolygonizeCellSync(node, source);
	});
	return r;
}

PolygonizeWorkerThreadData* TerrainPolygonizer::PolygonizeCellSync(ITerrainOctreeNode* cellToPolygonize, IVoxelDataSource* source)
{

	struct CellOutputHistory
	{
		u8 IndicesOutputted;
		u32* Indices;
	};

	CellOutputHistory cellsOutput[BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE];
	u32 cellOutputTop = 0;

	u8* data = (u8*)Allocator->Malloc(
		sizeof(PolygonizeWorkerThreadData) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainNormal) +
		TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainPosition) +
		TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32) +
		TOTAL_CELL_VOLUME_SIZE * sizeof(i8)
	); // malloc everything in a single block

	PolygonizeWorkerThreadData* rVal = (PolygonizeWorkerThreadData*)data;
	u8* dataPtr = data + sizeof(PolygonizeWorkerThreadData);


	rVal->Normals = (TerrainNormal*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainNormal);
	rVal->Positions = (TerrainPosition*)dataPtr;
	dataPtr += TERRAIN_CELL_VERTEX_ARRAY_SIZE * sizeof(TerrainPosition);
	rVal->Indices = (u32*)dataPtr;
	dataPtr += TERRAIN_CELL_INDEX_ARRAY_SIZE * sizeof(u32);
	rVal->VoxelData = (i8*)dataPtr;

	rVal->VerticesSize = TERRAIN_CELL_VERTEX_ARRAY_SIZE;
	rVal->IndicesSize = TERRAIN_CELL_INDEX_ARRAY_SIZE;
	rVal->Node = cellToPolygonize;
	rVal->OutputtedVertices = 0;
	rVal->OutputtedIndices = 0;
	rVal->MyAllocator = Allocator;

	u32 verticesTop = 0;
	u32 indicesTop = 0;

	source->GetVoxelsForNode(cellToPolygonize, rVal->VoxelData);

	// create polygons from voxel data here:
	auto flattenCellIndex = [](const glm::ivec3& coords) -> u32
	{
		// account for gutters
		const glm::ivec3 coordsToUse = coords + glm::ivec3{POLYGONIZER_NEGATIVE_GUTTER, POLYGONIZER_NEGATIVE_GUTTER, POLYGONIZER_NEGATIVE_GUTTER};
		// flatten. TODO: unify order of indices used in GetVoxelsForNode to the type used by the marching cubes algorithm below, eg for z for y for x with x*1 + y*2 + z*4
		return coordsToUse.x * TOTAL_SIDE_SIZE * TOTAL_SIDE_SIZE + coordsToUse.y * TOTAL_SIDE_SIZE + coordsToUse.z;
	};

	for (i32 z = 0; z < BASE_CELL_SIZE; z++)
	{
		for (i32 y=0; y < BASE_CELL_SIZE; y++)
		{
			for (i32 x = 0; x < BASE_CELL_SIZE; x++)
			{
				static i8 corner[8];

				// fill corner values - each corner of cell
				u8 caseIndex = 0;
				for (i32 cellZ = 0; cellZ < 2; cellZ++)
				{
					for (i32 cellY = 0; cellY < 2; cellY++)
					{
						for (i32 cellX = 0; cellX < 2; cellX++)
						{
							u32 cellI = cellZ * 4 + cellY * 2 + cellX;
							corner[cellI] = rVal->VoxelData[flattenCellIndex({ x + cellX, y + cellY, z + cellZ })];
						}
					}
				}
				
				// Bit twiddling trick page 18: https://transvoxel.org/Lengyel-VoxelTerrain.pdf
				unsigned long caseCode = ((corner[0] >> 7) & 0x01)
					| ((corner[1] >> 6) & 0x02)
					| ((corner[2] >> 5) & 0x04)
					| ((corner[3] >> 4) & 0x08)
					| ((corner[4] >> 3) & 0x10)
					| ((corner[5] >> 2) & 0x20)
					| ((corner[6] >> 1) & 0x40)
					| (corner[7] & 0x80);
				if ((caseCode ^ ((corner[7] >> 7) & 0xFF)) != 0)
				{
					// Cell has a nontrivial triangulation.

					// The RegularCellData structure holds information about the triangulation
					// used for a single equivalence class in the modified Marching Cubes algorithm,
					// described in Section 3.2.
					u8 cellClass = regularCellClass[caseCode];

					const RegularCellData& cellData = regularCellData[cellClass];
					const u16* vertexData = regularVertexData[caseCode];

					long numVerts = cellData.GetVertexCount();
					long numTris = cellData.GetTriangleCount();

					// maintain a history for vertex reuse
					CellOutputHistory& thisCell = cellsOutput[cellOutputTop];
					// starting point of indices
					thisCell.Indices = rVal->Indices;
					thisCell.IndicesOutputted = 0;
					
					for (i32 i = 0; i < numTris; i++)
					{
						for (i32 j = i * 3; i < i * 3 + 3; i++)
						{
							u16 thisVertex = vertexData[cellData.vertexIndex[i]];
							u8 edgeData = thisVertex & 0xff;

							// Each edge of a cell is assigned an 8-bit code, as shown in Figure 3.8(b),that provides a mapping to a preceding celland the coincident edge on that preceding cell for which new vertex creation was allowed.
							u8 vertexReuseData = (thisVertex >> 8);
							//The high nibble of this code indicates which direction to go in order to reach the correct preceding cell.
							u8 directionNibble = (vertexReuseData >> 4);

							/*
								The bit values 1, 2, and 4 in this nibble indicate that we must subtract
								one from the x, y, and /or z coordinate, respectively.These bits can be combined to indicate that
								the preceding cell is diagonally adjacent across an edge or across the minimal corner
							*/
							glm::ivec3 cellPtr =
							{
								-((directionNibble) | 1),
								-(((directionNibble) | 2) >> 1),
								-(((directionNibble) | 4) >> 2),
							};

							/*
								The low nibble of the 8 - bit
								code gives the index of the vertex in the preceding cell that should be reused or the index of the
								vertex in the current cell that should be created.
							*/
							u8 vertexIndex = vertexReuseData & 0x0f;

							/*
								.The bit value 8 indicates that a new vertex is to be created for the current cell.
							*/
							if (directionNibble | 8)
							{
								// create vertex

							}
							else
							{
								// reuse vertex

							}
						}
					}
				}
			}
		}
	}
	
	return rVal;
}
