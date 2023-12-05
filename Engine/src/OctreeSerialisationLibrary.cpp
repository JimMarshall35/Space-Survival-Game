#include "OctreeSerialisationLibrary.h"
#include "ITerrainOctreeNode.h"
#include "IVoxelDataSource.h"
#include "CommonTypedefs.h"
#include "TerrainDefs.h"
#include <cassert>
#include <iostream>
#include <fstream>

namespace OctreeSerialisation
{
#define CURRENT_FILE_FORMAT_VERSION 0
	struct VoxelFileHeader
	{
		u32 Version;
		u32 NumNodes;
	};

	void WriteHeader(std::ofstream& ofs, const std::unordered_set<TerrainOctreeIndex>& setNodes, IVoxelDataSource* voxelDataSource)
	{
		VoxelFileHeader header;
		header.Version = CURRENT_FILE_FORMAT_VERSION;
		header.NumNodes = setNodes.size();
		ofs.write((char*)&header, sizeof(VoxelFileHeader));
	}

#pragma optimize("", off)

	void WriteNode(std::ofstream& ofs, TerrainOctreeIndex index, ITerrainOctreeNode* node)
	{
		ofs.write((char*)&index, sizeof(TerrainOctreeIndex));
		ofs.write((const char*)node->GetVoxelData(), BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE);
	}

	void SaveNewlyGeneratedToFile(const std::unordered_set<TerrainOctreeIndex>& setNodes, IVoxelDataSource* voxelDataSource, const char* path)
	{
		std::ofstream ofs(path, std::ios::out | std::ofstream::binary);
		if(!ofs)
		{
			std::cout << "Cannot open file.\n";
			return;
		}
		WriteHeader(ofs, setNodes, voxelDataSource);
		for (TerrainOctreeIndex index : setNodes)
		{
			ITerrainOctreeNode* node = voxelDataSource->FindNodeFromIndex(index);
			assert(node->GetMipLevel() == 0);
			assert(node->GetVoxelData());
			WriteNode(ofs, index, node);
		}
	}
#pragma optimize("", on)

	void ReadHeader(std::ifstream& ifs, VoxelFileHeader& header)
	{
		ifs.read((char*)&header, sizeof(VoxelFileHeader));
	}

	void LoadFromFile(IVoxelDataSource* voxelDataSource, const char* path)
	{
		// 0x0000000005015200
		// 0x0000000001700514
		// 0x0000000004107410
		std::ifstream ifs(path, std::ios::in | std::ofstream::binary);
		VoxelFileHeader header;
		ReadHeader(ifs, header);
		if (header.Version != CURRENT_FILE_FORMAT_VERSION)
		{
			std::cout << "Trying to read a voxel file with an incorrect version\n";
		}

		std::vector<i8> debug(BASE_CELL_SIZE*BASE_CELL_SIZE*BASE_CELL_SIZE);
		for (int i = 0; i < header.NumNodes; i++)
		{
			TerrainOctreeIndex index;
			ifs.read((char*)&index, sizeof(TerrainOctreeIndex));
 			ITerrainOctreeNode* node = voxelDataSource->FindNodeFromIndex(index, true);
			assert(node);
			assert(node->GetMipLevel() == 0);
			voxelDataSource->AllocateNodeVoxelData(node);
			ifs.read((char*)debug.data(), BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE);
			memcpy(node->GetVoxelData(), debug.data(), BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE);
			//printf("hello");
		}
	}
}