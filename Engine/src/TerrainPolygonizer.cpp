#include "TerrainPolygonizer.h"
#include "IAllocator.h"
#include "TerrainDefs.h"
#include "IVoxelDataSource.h"
#include "TransVoxel.h"
#include "ITerrainOctreeNode.h"

#define TERRAIN_CELL_VERTEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum
#define TERRAIN_CELL_INDEX_ARRAY_SIZE 10000 // each worker can output this number of vertices maximum

#define TERRAIN_FIXED_FRACTION_SIZE_BITS 8
#define TERRAIN_FIXED_FRACTION_MAX 0xff

#define Q 8
#define K   (1 << (Q - 1))

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
		bool Occupied = false;
	};

	CellOutputHistory cellsOutputHistoryDeck0[BASE_DECK_SIZE];
	CellOutputHistory cellsOutputHistoryDeck1[BASE_DECK_SIZE];
	
	CellOutputHistory* currentFrontDeck = cellsOutputHistoryDeck0;
	CellOutputHistory* currentRearDeck = cellsOutputHistoryDeck1;

	CellOutputHistory* historyNextWritePtr = currentFrontDeck;

	auto writeToHistoryRecord([&historyNextWritePtr, &currentRearDeck, &currentFrontDeck]() -> CellOutputHistory&
	{
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

	auto readFromHistoryRecord([&currentRearDeck, &currentFrontDeck](const glm::ivec3& offset, const glm::ivec3& currentCell) -> CellOutputHistory&
	{
		
		CellOutputHistory* deckToUse;
		if (offset.z < 0)
		{
			deckToUse = currentRearDeck;
		}
		else 
		{
			deckToUse = currentFrontDeck;
		}
		return deckToUse[((currentCell.y + offset.y) * BASE_CELL_SIZE) + (currentCell.x + offset.x)];
	});

	
	
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

	auto GetVoxelValueAt = [rVal](u8 x, u8 y, u8 z) -> i8
	{
		return rVal->VoxelData[TOTAL_DECK_SIZE * z + TOTAL_CELL_SIZE * y + x];
	};


	u32 verticesTop = 0;
	u32 indicesTop = 0;

	source->GetVoxelsForNode(cellToPolygonize, rVal->VoxelData);

	
	glm::ivec3 blockBottomLeft = cellToPolygonize->GetBottomLeftCorner();

	for (i32 z = 0; z < BASE_CELL_SIZE; z++)
	{
		for (i32 y=0; y < BASE_CELL_SIZE; y++)
		{
			for (i32 x = 0; x < BASE_CELL_SIZE; x++)
			{
				static i8 corner[8];
				// maintain a history for vertex reuse
				CellOutputHistory& thisCellHistory = writeToHistoryRecord();
				thisCellHistory.Indices[0] = INT_MAX;
				thisCellHistory.Indices[1] = INT_MAX;
				thisCellHistory.Indices[2] = INT_MAX;
				thisCellHistory.Indices[3] = INT_MAX;

				thisCellHistory.Occupied = false;
				

				// fill corner values - each corner of cell
				u8 caseIndex = 0;
				for (u8 cellZ = 0; cellZ < 2; cellZ++)
				{
					for (u8 cellY = 0; cellY < 2; cellY++)
					{
						for (u8 cellX = 0; cellX < 2; cellX++)
						{
							u8 cellI = cellZ * 4 + cellY * 2 + cellX;
							corner[cellI] = GetVoxelValueAt(x + cellX, y + cellY, z + cellZ);
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
					
		 			auto ConstructValidityMask = [readFromHistoryRecord](const glm::ivec3& position) -> u8
					{
						u8 validityMask = 0;
						validityMask |= (u8)readFromHistoryRecord({ -1, 0, 0 }, position).Occupied << (u8)DirectionBitShift::X;
						validityMask |= (u8)readFromHistoryRecord({ 0, -1, 0 }, position).Occupied << (u8)DirectionBitShift::Y;
						validityMask |= (u8)readFromHistoryRecord({ 0, 0, -1 }, position).Occupied << (u8)DirectionBitShift::Z;
						return validityMask;
					};

					u32 validityMask = ConstructValidityMask({ x,y,z });

					for (i32 i = 0; i < numTris; i++)
					{
						for (i32 j = i * 3; j < (i * 3) + 3; j++)
						{
							u16 thisVertex = vertexData[cellData.vertexIndex[j]];
							u8 edgeCode = thisVertex & 0xff;

							// Each edge of a cell is assigned an 8-bit code, as shown in Figure 3.8(b),that provides a mapping to a preceding celland the coincident edge on that preceding cell for which new vertex creation was allowed.
							//The high nibble of this code indicates which direction to go in order to reach the correct preceding cell.
							u8 directionNibble = (thisVertex & 0x0F00) >> 8;

							/*
								The low nibble of the 8 - bit
								code gives the index of the vertex in the preceding cell that should be reused or the index of the
								vertex in the current cell that should be created.
							*/
							u8 vertexIndex = (thisVertex & 0xF000) >> 12;

							
							auto OutputVertexFixedPoint = [&]()
							{
								u16 v0 = (edgeCode >> 4) & 0x0F;
								u16 v1 = edgeCode & 0x0F;
								i32 d0 = corner[v0];
								i32 d1 = corner[v1];
								if (!(d1 - d0))
								{
									return;
								}
								i32 t = (d1 << 8) / (d1 - d0);////////////(d1 - d0) ? (d1 << 8) / (d1 - d0) : 0;
								static const glm::ivec3 unflattenToFloatLUT[8] = {
									glm::ivec3{0,0,0},
									glm::ivec3{1,0,0},
									glm::ivec3{0,1,0},
									glm::ivec3{1,1,0},
									glm::ivec3{0,0,1},
									glm::ivec3{1,0,1},
									glm::ivec3{0,1,1},
									glm::ivec3{1,1,1},
								};
								glm::ivec3 edgeOffset0 = unflattenToFloatLUT[v0];
								glm::ivec3 edgeOffset1 = unflattenToFloatLUT[v1];

								glm::ivec3 P0 =
								{
									(x + edgeOffset0.x) << 8,
									(y + edgeOffset0.y) << 8,
									(z + edgeOffset0.z) << 8
								};
								glm::ivec3 P1 =
								{
									(x + edgeOffset1.x) << 8,
									(y + edgeOffset1.y) << 8,
									(z + edgeOffset1.z) << 8
								};
								// Vertex lies in the interior of the edge.
								i32 u = 0x0100 - t;
								i32 thisVertexIndex = rVal->OutputtedVertices++;
								rVal->Indices[rVal->OutputtedIndices++] = thisVertexIndex;
								fixedPointVerts[thisVertexIndex].Position =
								{
									((t * P0.x) >> 8) + ((u * P1.x) >> 8),
									((t * P0.y) >> 8) + ((u * P1.y) >> 8),
									((t * P0.z) >> 8) + ((u * P1.z) >> 8),
								};
								thisCellHistory.Indices[vertexIndex] = thisVertexIndex;
								thisCellHistory.Occupied = true;
							};

							if (directionNibble == 8)
							{
								OutputVertexFixedPoint();
							}
							else if (directionNibble & validityMask)
							{
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
								if (cellToShare.Indices[vertexIndex] != INT_MAX)
								{
									rVal->Indices[rVal->OutputtedIndices++] = cellToShare.Indices[vertexIndex];
								}
								else
								{
									OutputVertexFixedPoint();
								}
							}
							else
							{
								OutputVertexFixedPoint();
							}
							/////////glm::ivec3 Q;
							//if ((t & 0x00FF) != 0)
							//{
							//	// Vertex lies in the interior of the edge.
							//	OutputVertexFixedPoint(t, P0, P1, vertexIndex);
							//}
							//else if (t == 0)
							//{
							//	// Vertex lies at the higher-numbered endpoint.
							//	if (v1 == 7)
							//	{
							//		// This cell owns the vertex.
							//		OutputVertexFixedPoint(t, P0, P1, vertexIndex);
							//	}
							//	else
							//	{
							//		// Try to reuse corner vertex from a preceding cell.
							//		if (directionNibble & validityMask)
							//		{
							//			/*
							//				The bit values 1, 2, and 4 in this nibble indicate that we must subtract
							//				one from the x, y, and /or z coordinate, respectively.These bits can be combined to indicate that
							//				the preceding cell is diagonally adjacent across an edge or across the minimal corner
							//			*/
							//			glm::ivec3 direction = {
							//				-(directionNibble & 1),
							//				-((directionNibble & 2) >> 1),
							//				-((directionNibble & 4) >> 2)

							//			};
							//			CellOutputHistory& cellToShare = readFromHistoryRecord(direction, { x,y,z });
							//			rVal->Indices[rVal->OutputtedIndices++] = cellToShare.Indices[vertexIndex];
							//		}
							//		else
							//		{
							//			// create vertex
							//			OutputVertexFixedPoint(t, P0, P1, vertexIndex);
							//		}
							//	}
							//}
							//else
							//{
							//	// Vertex lies at the lower-numbered endpoint.
							//	// Always try to reuse corner vertex from a preceding cell.
							//	if (directionNibble & validityMask)
							//	{
							//		/*
							//			The bit values 1, 2, and 4 in this nibble indicate that we must subtract
							//			one from the x, y, and /or z coordinate, respectively.These bits can be combined to indicate that
							//			the preceding cell is diagonally adjacent across an edge or across the minimal corner
							//		*/
							//		glm::ivec3 direction = {
							//			-(directionNibble & 1),
							//			-((directionNibble & 2) >> 1),
							//			-((directionNibble & 4) >> 2)

							//		};
							//		CellOutputHistory& cellToShare = readFromHistoryRecord(direction, { x,y,z });
							//		rVal->Indices[rVal->OutputtedIndices++] = cellToShare.Indices[vertexIndex];
							//	}
							//	else
							//	{
							//		// create vertex
							//		OutputVertexFixedPoint(t, P0, P1, vertexIndex);
							//	}
							//}
						}
					}
				}
			}
		}
	}

	auto ConvertVertexFixedToFloat = [cellToPolygonize, blockBottomLeft](u32 startingPoint, u32 numberToConvert, const TerrainVertexFixedPoint* fixedPointVertsIn, TerrainVertex* floatingPointVertsOut)
	{
		auto fixedToFloat = [](u32 fixed8) -> float
		{
			return float(fixed8) / (1 << 8);
		};
		float sizeInVoxels = cellToPolygonize->GetSizeInVoxels();
		const glm::vec3 blFloat = blockBottomLeft;
		for (i32 i = startingPoint; i < numberToConvert; i++)
		{
			TerrainVertex& floatVert = floatingPointVertsOut[i];
			const TerrainVertexFixedPoint& fixedVert = fixedPointVertsIn[i];
			

			floatVert.Position =
			{
				// whole number part                 // fractional part
				/*(float)(fixedVert.Position.x >> 8) + ((float)fixedVert.Position.x / (float)TERRAIN_FIXED_FRACTION_MAX),
				(float)(fixedVert.Position.y >> 8) + ((float)fixedVert.Position.y / (float)TERRAIN_FIXED_FRACTION_MAX),
				(float)(fixedVert.Position.z >> 8) + ((float)fixedVert.Position.z / (float)TERRAIN_FIXED_FRACTION_MAX),*/
				fixedToFloat(fixedVert.Position.x),
				fixedToFloat(fixedVert.Position.y),
				fixedToFloat(fixedVert.Position.z),

			};

			floatVert.Position = floatVert.Position * (sizeInVoxels / BASE_CELL_SIZE);
			floatVert.Position += blFloat;

			floatVert.Normal =
			{
				(float)(fixedVert.Normal.x >> 8) + ((float)fixedVert.Normal.x / (float)TERRAIN_FIXED_FRACTION_MAX),
				(float)(fixedVert.Normal.y >> 8) + ((float)fixedVert.Normal.y / (float)TERRAIN_FIXED_FRACTION_MAX),
				(float)(fixedVert.Normal.z >> 8) + ((float)fixedVert.Normal.z / (float)TERRAIN_FIXED_FRACTION_MAX),
			};
		}
	};

	ConvertVertexFixedToFloat(0, rVal->OutputtedVertices, fixedPointVerts, rVal->Vertices);
	
	return rVal;
}
