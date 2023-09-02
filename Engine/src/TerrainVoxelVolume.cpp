#include "TerrainVoxelVolume.h"
#include <glm.hpp>
#include "Gizmos.h"
#include "IAllocator.h"

RunLengthT TerrainVoxelVolume::RLEByteArray::LengthOfDataEncoded;
IAllocator* TerrainVoxelVolume::RLEByteArray::Allocator;

TerrainVoxelVolume::TerrainVoxelVolume(IAllocator* allocator, u32 dimensionsInVoxels, i8 initialValue)
    : DimensionsInVoxels(dimensionsInVoxels),
    Allocator(allocator),
    ByteArray2dArray(IAllocator::NewArray<RLEByteArray>(Allocator, DimensionsInVoxels* DimensionsInVoxels))
{
    RLEByteArray::Allocator = Allocator;
    InitializeArrays(initialValue);
}

void TerrainVoxelVolume::DebugVisualiseVolume()
{
    float halfDimensionsInVoxels = DimensionsInVoxels/ 2;
    glm::vec3 center = { halfDimensionsInVoxels, halfDimensionsInVoxels, halfDimensionsInVoxels };
    glm::vec3 dims = { DimensionsInVoxels, halfDimensionsInVoxels, halfDimensionsInVoxels };
    Gizmos::AddBox(center, dims, false);
}

ivec3 TerrainVoxelVolume::GetTerrainTotalDims()
{
    return ivec3(DimensionsInVoxels, DimensionsInVoxels, DimensionsInVoxels);
}

void TerrainVoxelVolume::InitializeArrays(i8 initialValue)
{
    assert(DimensionsInVoxels <= EncodedDataRowMaxSize);

    for (u32 x = 0; x < DimensionsInVoxels; x++)
    {
        for (u32 y = 0; y < DimensionsInVoxels; y++)
        {
            // allocate the array to a certain size, when the actual used data exceeds this
            // it'll realloc to double the size
            const u32 initialAllocationSize = 32;
            RLEByteArray& byteArray = ByteArray2dArray[x + y * DimensionsInVoxels];
            byteArray.CurrentAllocatedLength = initialAllocationSize;
            byteArray.RLEArray = IAllocator::NewArray<i8>(Allocator, initialAllocationSize);
            
            // create buffer containing the first and only run of the initialised RLE array
            i8 buffer[RLERunSize];
            *((RunLengthT*)buffer) = DimensionsInVoxels;
            buffer[sizeof(RunLengthT)] = initialValue;
            
            // copy buffer to the new array
            for (byteArray.CurrentOccupiedLength = 0; byteArray.CurrentOccupiedLength < RLERunSize; byteArray.CurrentOccupiedLength++)
            {
                byteArray.RLEArray[byteArray.CurrentOccupiedLength] = buffer[byteArray.CurrentOccupiedLength];
            }
        }
    }
}

void TerrainVoxelVolume::SetValue(const glm::uvec3& coordinate, i8 newValue)
{
    assert(coordinate.x < DimensionsInVoxels);
    assert(coordinate.y < DimensionsInVoxels);
    assert(coordinate.z < DimensionsInVoxels);
    ByteArray2dArray[coordinate.x + coordinate.y * DimensionsInVoxels]
        .SetValue(coordinate.z, newValue);
}

i8 TerrainVoxelVolume::GetValue(const glm::uvec3& coordinate)
{
    assert(coordinate.x < DimensionsInVoxels);
    assert(coordinate.y < DimensionsInVoxels);
    assert(coordinate.z < DimensionsInVoxels);
    return ByteArray2dArray[coordinate.x + coordinate.y * DimensionsInVoxels]
        .GetValue(coordinate.z);
}

void TerrainVoxelVolume::RLEByteArray::SetValue(size_t coordinate, i8 newValue)
{
    u32 totalLengthOfDataTravelled = 0;
    u32 i = 0;
    while (i < CurrentOccupiedLength)
    {
        u32 previousI = i;
        Run run = DecodeRun(i, &totalLengthOfDataTravelled);
        if (totalLengthOfDataTravelled >= coordinate)
        {
            if (run.Length == 1)
            {
                RLEArray[previousI + sizeof(RunLengthT)] = newValue;
                return;
            }
            else if(run.Value != newValue)
            {
                RunLengthT voxelsAfterCoord = totalLengthOfDataTravelled - coordinate;
                RunLengthT& foundInRunLength = *((RunLengthT*)&RLEArray[previousI]);

                // Edge cases:
                // if we change a voxel on either edge of a run, it needs to merge with the one
                // next to it if that happens to be same the value we're change it to to preserve the RLE
                if (voxelsAfterCoord == run.Length)
                {
                    RunLengthT& previousRunLength = (*(RunLengthT*)RLEArray[previousI - RLERunSize]);
                    i8 previousRunValue = RLEArray[(previousI - RLERunSize) + sizeof(RunLengthT)];
                    if (previousRunValue == newValue)
                    {
                        previousRunLength++;
                        foundInRunLength--;
                        return;
                    }
                }
                else if (voxelsAfterCoord == 0)
                {
                    RunLengthT& nextRunLength = (*(RunLengthT*)RLEArray[previousI]);
                    i8 nextRunValue = RLEArray[previousI + sizeof(RunLengthT)];
                    if (nextRunValue == newValue)
                    {
                        nextRunLength++;
                        foundInRunLength--;
                        return;
                    }
                }

                RunLengthT voxelsBeforeCoord = run.Length - voxelsAfterCoord;
                foundInRunLength--;
                size_t occupiedDataRemaining = CurrentOccupiedLength;
                size_t newRequiredData = occupiedDataRemaining + RLERunSize;

                if (newRequiredData > CurrentAllocatedLength)
                {
                    RLEArray = (i8*)Allocator->Realloc(RLEArray, CurrentAllocatedLength * 2);
                }

                void* copyStart = &RLEArray[i];
                void* copyDest = (u8*)copyStart + RLERunSize;
                memcpy(copyDest, copyStart, occupiedDataRemaining);
                *((RunLengthT*)copyStart) = 1;
                ((i8*)copyStart)[sizeof(RunLengthT)] = newValue;

                return;
            }
        }
    }
}

i8 TerrainVoxelVolume::RLEByteArray::GetValue(size_t coordinate)
{
    assert(CurrentOccupiedLength <= CurrentAllocatedLength);
    assert(coordinate < CurrentOccupiedLength);
    u32 totalLengthOfDataTravelled = 0;
    u32 i = 0;
    while (i < CurrentOccupiedLength)
    {
        Run run = DecodeRun(i, &totalLengthOfDataTravelled);
        if (totalLengthOfDataTravelled >= coordinate)
        {
            return run.Value;
        }
    }
    assert(false); // should never get here
    return i8();
}

TerrainVoxelVolume::RLEByteArray::Run TerrainVoxelVolume::RLEByteArray::DecodeRun(u32& ptr, u32* totalLengthTravelled)
{
    // copy the run at index ptr to the buffer
    i8 buffer[RLERunSize];
    for (ptr; ptr < RLERunSize; ptr++)
    {
        buffer[ptr] = RLEArray[ptr];
    }

    // decode from the buffer to a Run struct to be returned
    Run r;
    r.Length = *((RunLengthT*)buffer);
    r.Value = buffer[sizeof(RunLengthT)];

    // increment totalLengthTravelled if it has been set
    if (totalLengthTravelled)
    {
        *totalLengthTravelled = (*totalLengthTravelled) + r.Length;
    }

    return r;
}
