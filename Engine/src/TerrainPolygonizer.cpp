#include "TerrainPolygonizer.h"
#include "IAllocator.h"
#include "TerrainDefs.h"
#include "IVoxelDataSource.h"
#include "TransVoxel.h"
#include "ITerrainOctreeNode.h"

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
	struct CellOutputHistory
	{
		u32 Indices[4];
	};

	CellOutputHistory cellsOutputHistoryDeck0[BASE_DECK_SIZE];
	CellOutputHistory cellsOutputHistoryDeck1[BASE_DECK_SIZE];
	
	CellOutputHistory* currentFrontDeck = cellsOutputHistoryDeck0;
	CellOutputHistory* currentRearDeck = cellsOutputHistoryDeck1;

	CellOutputHistory* historyNextWritePtr = currentFrontDeck;

	auto writeToHistoryRecord([&historyNextWritePtr, &currentRearDeck, &currentFrontDeck]() -> CellOutputHistory& {
		if (historyNextWritePtr + 1 >= currentFrontDeck + BASE_DECK_SIZE)
		{
			// swap decks
			auto tempDeck = currentFrontDeck;
			currentFrontDeck = currentRearDeck;
			currentRearDeck = tempDeck;
			historyNextWritePtr = currentFrontDeck;
		}
		return *historyNextWritePtr++;
	});

	auto readFromHistoryRecord([&currentRearDeck, &currentFrontDeck](const glm::ivec3& offset, const glm::ivec3& currentCell) -> CellOutputHistory& {
		
		CellOutputHistory* deckToUse;
		if (offset.z < 0)
		{
			deckToUse = currentRearDeck;
		}
		else 
		{
			deckToUse = currentFrontDeck;
		}
		return deckToUse[(currentCell.y + offset.y * BASE_CELL_SIZE) + (currentCell.x + offset.x)];
	});
	
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

	
	glm::vec3 blockBottomLeft = cellToPolygonize->GetBottomLeftCorner();

	for (i32 z = 0; z < BASE_CELL_SIZE; z++)
	{
		for (i32 y=0; y < BASE_CELL_SIZE; y++)
		{
			for (i32 x = 0; x < BASE_CELL_SIZE; x++)
			{
				static i8 corner[8];
				// maintain a history for vertex reuse
				CellOutputHistory& thisCellHistory = writeToHistoryRecord();
				

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

					enum class DirectionBitMask : u8
					{
						X = 1,
						Y = 2,
						Z = 4
					};

					enum class DirectionBitShift : u8
					{
						X = 0,
						Y = 1,
						Z = 2
					};
					// set validity mask

					// The RegularCellData structure holds information about the triangulation
					// used for a single equivalence class in the modified Marching Cubes algorithm,
					// described in Section 3.2.
					u8 cellClass = regularCellClass[caseCode];

					const RegularCellData& cellData = regularCellData[cellClass];
					const u16* vertexData = regularVertexData[caseCode];

					long numVerts = cellData.GetVertexCount();
					long numTris = cellData.GetTriangleCount();

					//thisCellHistory.IndicesOutputted = numTris * 3;


					/*
						For cells occurring along the minimal boundaries of a block, the preceding cells needed for
						vertex reuse may not exist. In these cases, we allow new vertex creation on additional edges of a
						cell. While iterating through the cells in a block, a 3-bit mask is maintained whose bits indicate
						whether corresponding bits in a direction code are valid.
					*/
					u8 validityMask = 0;
					validityMask |= ((u8)(x > 0)) << (u32)DirectionBitShift::X;
					validityMask |= ((u8)(y > 0)) << (u32)DirectionBitShift::Y;
					validityMask |= ((u8)(z > 0)) << (u32)DirectionBitShift::Z;

					for (i32 i = 0; i < numTris; i++)
					{
						for (i32 j = i * 3; j < (i * 3) + 3; j++)
						{
							u16 thisVertex = vertexData[cellData.vertexIndex[j]];
							u8 edgeData = thisVertex & 0xff;

							// Each edge of a cell is assigned an 8-bit code, as shown in Figure 3.8(b),that provides a mapping to a preceding celland the coincident edge on that preceding cell for which new vertex creation was allowed.
							u8 vertexReuseData = (thisVertex >> 8);
							//The high nibble of this code indicates which direction to go in order to reach the correct preceding cell.
							u8 directionNibble = (vertexReuseData >> 4);

							

							/*
								The low nibble of the 8 - bit
								code gives the index of the vertex in the preceding cell that should be reused or the index of the
								vertex in the current cell that should be created.
							*/
							u8 vertexIndex = vertexReuseData & 0x0f;

							/*
								.The bit value 8 indicates that a new vertex is to be created for the current cell.
							*/
							if ((directionNibble | 8) || (directionNibble & validityMask))
							{
								// create vertex

								/*
								TODO:
								the true giga-chad marching cubes implementer such as Lengyel keeps everything
								as integers in this part of the algorithm to do the interpolation in fixed point,
								and outputs fixed point fractions in the space of the cube, to convert to floating
								point later on which would include adding the block bottom left corner then.
								See listing 3.2
								*/
								u8 pointIndex0 = edgeData >> 4;
								u8 pointIndex1 = edgeData & 0x0f;
								static const glm::vec3 unflattenToFloatLUT[8] = {
									glm::vec3{0,0,0},
									glm::vec3{1,0,0},
									glm::vec3{0,1,0},
									glm::vec3{1,1,0},
									glm::vec3{0,0,1},
									glm::vec3{1,0,1},
									glm::vec3{0,1,1},
									glm::vec3{1,1,1},
								};
								glm::vec3 cellBL = blockBottomLeft + glm::vec3{ x, y, z };
								glm::vec3 point0 = cellBL + unflattenToFloatLUT[pointIndex0];
								glm::vec3 point1 = cellBL + unflattenToFloatLUT[pointIndex1];
								i8 value0 = corner[pointIndex0];
								i8 value1 = corner[pointIndex1];

								float t = (float)value1 - ((float)value1 - (float)value0);

								auto getNormal = [&flattenCellIndex](const glm::ivec3& corner0, glm::ivec3& corner1, float t, i8* voxelData) -> glm::vec3
								{
									glm::vec3 normal0 = glm::normalize(glm::vec3
										{
											voxelData[flattenCellIndex(corner0 + glm::ivec3{-1, 0, 0})] - voxelData[flattenCellIndex(corner0 + glm::ivec3{1, 0, 0})],
											voxelData[flattenCellIndex(corner0 + glm::ivec3{0, -1, 0})] - voxelData[flattenCellIndex(corner0 + glm::ivec3{0, 1, 0})],
											voxelData[flattenCellIndex(corner0 + glm::ivec3{0, 0, -1})] - voxelData[flattenCellIndex(corner0 + glm::ivec3{0, 0, 1})],
										});

									glm::vec3 normal1 = glm::normalize(glm::vec3
										{
											voxelData[flattenCellIndex(corner1 + glm::ivec3{-1, 0, 0})] - voxelData[flattenCellIndex(corner1 + glm::ivec3{1, 0, 0})],
											voxelData[flattenCellIndex(corner1 + glm::ivec3{0, -1, 0})] - voxelData[flattenCellIndex(corner1 + glm::ivec3{0, 1, 0})],
											voxelData[flattenCellIndex(corner1 + glm::ivec3{0, 0, -1})] - voxelData[flattenCellIndex(corner1 + glm::ivec3{0, 0, 1})],
										});

									return glm::mix(normal0, normal1, t);
								};

								glm::vec3 position = t * point0 + (1 - t) * point1;
								glm::vec3 normal = getNormal(glm::ivec3(point0), glm::ivec3(point1), t, rVal->VoxelData);
								u32 thisVertIndex = rVal->OutputtedVertices;
								rVal->Positions[rVal->OutputtedVertices] = position;
								rVal->Normals[rVal->OutputtedVertices++] = normal;
								rVal->Indices[rVal->OutputtedIndices++] = thisVertIndex;
								thisCellHistory.Indices[vertexIndex] = thisVertIndex;
								// todo: some checking here + general asserts throughout

							}
							else
							{
								// reuse vertex
								
								/*
									When a direction code is used to locate a
									preceding cell, it is first ANDed with the validity mask to determine whether the preceding cell
									exists..., and if not, the creation of a new vertex in the current cell is permitted.
								*/
								/*
									The bit values 1, 2, and 4 in this nibble indicate that we must subtract
									one from the x, y, and /or z coordinate, respectively.These bits can be combined to indicate that
									the preceding cell is diagonally adjacent across an edge or across the minimal corner
								*/
								glm::ivec3 direction = {
									-(directionNibble & 1),
									-((directionNibble & 2) >> 1),
									-((directionNibble & 4) >> 2)

								};
								CellOutputHistory& cellToShare = readFromHistoryRecord(direction, { x,y,z });
								rVal->Indices[rVal->OutputtedIndices++] = cellToShare.Indices[vertexIndex];
								
							}
						}
					}
				}
			}
		}
	}
	
	return rVal;
}
