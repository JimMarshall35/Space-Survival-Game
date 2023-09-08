#include "TestProceduralTerrainVoxelPopulator.h"
#include "CommonTypedefs.h"
#include "IVoxelDataSource.h"

void TestProceduralTerrainVoxelPopulator::PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo)
{
	u32 size = dataSrcToWriteTo->GetSize();
	for (i32 z = 0; z < 200; z++)
	{
		for (i32 y = 0; y < size; y++)
		{
			for (i32 x = 0; x < size; x++)
			{
				i8 value;
				value = (((float)y - 1024.0f) / 1024.0f) * 127.0f;
				dataSrcToWriteTo->SetVoxelAt({ x,y,z }, value);
			}
		}
	}
}
