#include "ITerrainVoxelPopulator.h"
#include <memory>

class ITerrainVoxelPopulator;
namespace rdx
{
	class thread_pool;
}

class TestProceduralTerrainVoxelPopulator : public ITerrainVoxelPopulator
{
public:
	TestProceduralTerrainVoxelPopulator(const std::shared_ptr<rdx::thread_pool>& threadPool);
	// Inherited via ITerrainVoxelPopulator
	virtual void PopulateTerrain(IVoxelDataSource* dataSrcToWriteTo) override;
private:
	std::shared_ptr<rdx::thread_pool> ThreadPool;
};