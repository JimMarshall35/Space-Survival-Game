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
			WriteNode(ofs, index, node);
		}
	}

	void ReadHeader(std::ifstream& ifs, VoxelFileHeader& header)
	{
		ifs.read((char*)&header, sizeof(VoxelFileHeader));
	}

	void LoadFromFile(IVoxelDataSource* voxelDataSource, const char* path)
	{
		std::ifstream ifs(path, std::ios::in | std::ofstream::binary);
		VoxelFileHeader header;
		ReadHeader(ifs, header);
		if (header.Version != CURRENT_FILE_FORMAT_VERSION)
		{
			std::cout << "Trying to read a voxel file with an incorrect version\n";
		}

		for (int i = 0; i < header.NumNodes; i++)
		{
			TerrainOctreeIndex index;
			ifs.read((char*)&index, sizeof(TerrainOctreeIndex));
			ITerrainOctreeNode* node = voxelDataSource->FindNodeFromIndex(index, true);
			if (node->GetVoxelData() == nullptr)
			{
				voxelDataSource->AllocateNodeVoxelData(node);
			}
			ifs.read((char*)node->GetVoxelData(), BASE_CELL_SIZE * BASE_CELL_SIZE * BASE_CELL_SIZE);
			//printf("hello");
		}
	}
}