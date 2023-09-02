#pragma once
#include "ITerrainVoxelVolume.h"
#include "CommonTypedefs.h"
#include "Core.h"

#define DefaultDimensionsInVoxels 2048
#define DefaultInitialValue 50

typedef u16 RunLengthT; // maximum length of rle encoded data
#define EncodedDataRowMaxSize USHRT_MAX

#define RLERunSize (sizeof(RunLengthT) + sizeof(i8))

class IAllocator;
class APP_API TerrainVoxelVolume : public ITerrainVoxelVolume
{
private:
	struct RLEByteArray
	{
		struct Run
		{
			RunLengthT Length;
			i8 Value;
		};
		i8* RLEArray;
		RunLengthT CurrentAllocatedLength;
		RunLengthT CurrentOccupiedLength;
		static RunLengthT LengthOfDataEncoded;
		static IAllocator* Allocator;
		void SetValue(size_t coordinate, i8 newValue);
		i8 GetValue(size_t coordinate);
		Run DecodeRun(u32& ptr, u32* totalLengthTravelled = nullptr);
	};
public:
	TerrainVoxelVolume(IAllocator* allocator, u32 dimensionsInVoxels=DefaultDimensionsInVoxels, i8 initialValue= DefaultInitialValue);
	void DebugVisualiseVolume();
	// Inherited via ITerrainVoxelVolume
	virtual ivec3 GetTerrainTotalDims() override;
	
	void InitializeArrays(i8 initialValue);

	void SetValue(const glm::uvec3& coordinate, i8 newValue);
	i8 GetValue(const glm::uvec3& coordinate);

private:
	u32 DimensionsInVoxels = 2048;
	IAllocator* Allocator;
	RLEByteArray* ByteArray2dArray;
};