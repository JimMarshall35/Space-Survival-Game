#include <iostream>
#include "BasicHeapAllocator.h"
#include <type_traits>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>

enum TestCmdType
{
	Malloc,
	Free,
	Realloc,
	Print,
	VerifyNumBlocks,
	VerifyBlockCapacities,
	VerifyBlockCurrentSizes,
	VerifyBlocksAreFree,
};

struct ReallocData
{
	u32 AllocationIndex;
	u32 NewSize;
};
union TestCmdData {
	u32 UnsignedInt;
	ReallocData ReallocData;
	std::vector<size_t> Sizes;
	std::vector<bool> AreFree;
};
struct TestCmd
{
	TestCmdType type;
	TestCmdData Data;
};

class BasicHeapAllocatorTestHarness
{
private:
	static int CountBlocks(const BasicHeap& heap)
	{
		BasicHeapBlockHeader* onBlock = heap.BlocksListHead;
		int count = 0;
		while (onBlock)
		{
			++count;
			onBlock = onBlock->Next;
		}
		return count;
	}
public:


	static bool DoTestProgram(const std::vector<TestCmd>& program, BasicHeap& heap, std::string& errorMsg)
	{
		std::stringstream error;
		bool testSuccess = true;
		std::vector<void*> allocations;
		int onCmdIndex = 0;
		for (const TestCmd& cmd : program)
		{
			switch (cmd.type)
			{
			case Malloc:
				{
					allocations.push_back(heap.Malloc(cmd.Data.UnsignedInt));
				}
				break; 
			case Free:
				{
					u32 index = cmd.Data.UnsignedInt;
					if (index < allocations.size())
					{
						heap.Free(allocations[index]);
					}
					else
					{
						testSuccess = false;
						error << "cmd " << onCmdIndex << "\n";
						error << "TEST PROGRAM ERROR - Free index: " << index << " is not a valid index in allocations. allocations length" << allocations.size() << " \n";
					}
				}
				break; 
			case Realloc:
				{
					ReallocData reallocData = cmd.Data.ReallocData;
					if (reallocData.AllocationIndex < allocations.size())
					{
						void* ptr = allocations[reallocData.AllocationIndex];
						size_t newSize = reallocData.NewSize;
						allocations[reallocData.AllocationIndex] = heap.Realloc(ptr, newSize);
					}
					else
					{
						testSuccess = false;
						error << "cmd " << onCmdIndex << "\n";
						error << "TEST PROGRAM ERROR - Realloc index: " << reallocData.AllocationIndex << " is not a valid index in allocations. allocations length" << allocations.size() << " \n";
					}
				}
				break; 
			case Print:
				{
					heap.DebugPrintAllBlocks();
				}
				break;
			case VerifyNumBlocks:
				{
					int i = CountBlocks(heap);
					if (i != cmd.Data.UnsignedInt)
					{
						testSuccess = false;
						error << "cmd " << onCmdIndex << "\n";
						error << "expected size " << cmd.Data.UnsignedInt << " does not equal actual size " << i << "\n";
					}
				}
				break;
			case VerifyBlockCapacities:
				{
					int i = 0;
					BasicHeapBlockHeader* onBlock = heap.BlocksListHead;
					const std::vector<size_t>& expectedSizes = cmd.Data.Sizes;

					while (onBlock)
					{
						if (i >= expectedSizes.size())
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "Number of blocks does not match size of expected capacities. Actual:  " << CountBlocks(heap) << ". Expected: " << expectedSizes.size() << "\n";
							break;
						}
						if (onBlock->Capacity != expectedSizes[i])
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "capacity of block " << i << " (" << onBlock->Capacity << ")" << " doesn't match expected ("<< expectedSizes[i] <<")\n";
						}

						i++;
						onBlock = onBlock->Next;
					}
				}
				break;
			case VerifyBlockCurrentSizes:
				{
					int i = 0;
					BasicHeapBlockHeader* onBlock = heap.BlocksListHead;
					const std::vector<size_t>& expectedSizes = cmd.Data.Sizes;

					while (onBlock)
					{
						if (i >= expectedSizes.size())
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "Number of blocks does not match size of expected currentSizes. Actual:  " << CountBlocks(heap) << ". Expected: " << expectedSizes.size() << "\n";
							break;
						}
						if (onBlock->CurrentSize != expectedSizes[i])
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "current size of block " << i << " (" << onBlock->CurrentSize << ")" << " doesn't match expected (" << expectedSizes[i] << ")\n";
						}

						i++;
						onBlock = onBlock->Next;
					}
				}
				break;
			case VerifyBlocksAreFree:
				{
					int i = 0;
					BasicHeapBlockHeader* onBlock = heap.BlocksListHead;
					const std::vector<bool>& expectedIsFree = cmd.Data.AreFree;

					while (onBlock)
					{
						if (i >= expectedIsFree.size())
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "Number of blocks does not match size of expected AreFree values. Actual:  " << CountBlocks(heap) << ". Expected: " << expectedIsFree.size() << "\n";
							break;
						}
						if (onBlock->bDataIsFree != expectedIsFree[i])
						{
							testSuccess = false;
							error << "cmd " << onCmdIndex << "\n";
							error << "freedom of block " << i << " (" << onBlock->bDataIsFree << ")" << " doesn't match expected (" << expectedIsFree[i] << ")\n";
						}

						i++;
						onBlock = onBlock->Next;
					}
				}
				 break;
			}
			++onCmdIndex;
		}
		errorMsg = error.str();
		return testSuccess;
	}
};


void Test1()
{
	//BasicHeap heap(1024, "test heap");
	//void* a1 = heap.Malloc(32);
	//void* a2 = heap.Malloc(256);
	//void* a3 = heap.Malloc(21);
	//void* a4 = heap.Malloc(32);
	////memset(a1, '1', 32);
	////memset(a2, '2', 42);
	////memset(a3, '3', 21);
	////memset(a4, '4', 32);
	//heap.DebugPrintAllBlocks();
	//heap.Free(a2);
	//heap.DebugPrintAllBlocks();
	//void* a5 = heap.Malloc(6);
	//heap.DebugPrintAllBlocks();
	//heap.Free(a3);
	//heap.DebugPrintAllBlocks();
	
	std::vector<TestCmd> testProgram;
	TestCmd t = { Malloc, 32 };
	testProgram.push_back(t);

}
int main()
{
	Test1();
	return 0;
}