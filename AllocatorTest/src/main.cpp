#include <iostream>
#include "BasicHeapAllocator.h"
#include <type_traits>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <variant>

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
	VerifyTopOfMemory,
};

struct ReallocData
{
	u32 AllocationIndex;
	u32 NewSize;
};

typedef std::variant < u32, ReallocData, std::vector<size_t>, std::vector<bool>> TestCmdData;
struct TestCmd
{
	TestCmdType type;
	TestCmdData Data;
};

struct TestProgram
{
	std::string TestName;
	u32 HeapSize;
	std::vector<TestCmd> Commands;
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


	static bool DoTestProgram(const TestProgram& program, BasicHeap& heap, std::string& errorMsg)
	{
		std::stringstream error;
		bool testSuccess = true;
		std::vector<void*> allocations;
		int onCmdIndex = 0;
		std::cout << "Running test: " << program.TestName << "\n";
		for (const TestCmd& cmd : program.Commands)
		{
			switch (cmd.type)
			{
			case Malloc:
				{
					allocations.push_back(heap.Malloc(std::get<u32>(cmd.Data)));//cmd.Data.UnsignedInt));
				}
				break; 
			case Free:
				{
					u32 index = std::get<u32>(cmd.Data);
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
					const ReallocData& reallocData = std::get<ReallocData>(cmd.Data);
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
					if (i != std::get<u32>(cmd.Data))
					{
						testSuccess = false;
						error << "cmd " << onCmdIndex << "\n";
						error << "expected size " << std::get<u32>(cmd.Data) << " does not equal actual size " << i << "\n";
					}
				}
				break;
			case VerifyBlockCapacities:
				{
					int i = 0;
					BasicHeapBlockHeader* onBlock = heap.BlocksListHead;
					const std::vector<size_t>& expectedSizes = std::get<std::vector<size_t>>(cmd.Data);

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
					const std::vector<size_t>& expectedSizes = std::get<std::vector<size_t>>(cmd.Data);

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
					const std::vector<bool>& expectedIsFree = std::get<std::vector<bool>>(cmd.Data);

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
			case VerifyTopOfMemory:
				{
					u32 topOfDataOffset = std::get<u32>(cmd.Data);
					if (heap.TopOfUsedMemory != (u8*)heap.RawMemory + topOfDataOffset)
					{
						testSuccess = false;
						error << "cmd " << onCmdIndex << "\n";
						error << "expected top of memory offset: " << topOfDataOffset << ". Actual: " << heap.TopOfUsedMemory - (u8*)heap.RawMemory;
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

void RunTestPrograms(const std::vector<TestProgram>& programs)
{
	std::vector<std::string> failedTests;
	for (const TestProgram& prog : programs)
	{
		std::string error;
		BasicHeap heap(prog.HeapSize, prog.TestName);
		if (BasicHeapAllocatorTestHarness::DoTestProgram(prog, heap, error))
		{
			std::cout << "test succeeded\n";
		}
		else
		{
			failedTests.push_back(prog.TestName);
			std::cout << "test failed\n";
			std::cout << error;
		}
	}
	if (failedTests.size() > 0)
	{
		std::cout << "Some tests failed: \n";
		for (const std::string& testName : failedTests)
		{
			std::cout << "\t" << testName << "\n";
		}
	}
}

std::vector<TestProgram> gTestPrograms;

#define PROGRAM(name, heapSize) gTestPrograms.push_back(TestProgram{name, heapSize})

void CMD(TestCmdType type, u32 data)
{
	gTestPrograms[gTestPrograms.size() - 1].Commands.push_back(TestCmd{ type, data });
}

void CMD(TestCmdType type, const ReallocData& data)
{
	gTestPrograms[gTestPrograms.size() - 1].Commands.push_back(TestCmd{ type, data });
}


void CMD(TestCmdType type, const std::vector<size_t>& data)
{
	TestCmd cmd;
	cmd.type = type;
	std::vector<size_t> vec;
	cmd.Data.emplace<2>(data); // <2> for second member of variant i.e. std::vector<size_t>... weird variant API...
	gTestPrograms[gTestPrograms.size() - 1].Commands.push_back(cmd);
}

#define PRINT gTestPrograms[gTestPrograms.size() - 1].Commands.push_back(TestCmd{Print})

void BuildProgramList()
{
	PROGRAM("test1 - correct number of blocks", 1024);
		CMD(Malloc, 32);
		CMD(Malloc, 256);
		CMD(Malloc, 21);
		CMD(Malloc, 32);
		//PRINT;
		CMD(VerifyNumBlocks, 4);
		CMD(Free, 1);
		//PRINT;
		CMD(VerifyNumBlocks, 4);
		CMD(Malloc, 6);
		CMD(VerifyNumBlocks, 5);
		CMD(Free, 2);
		CMD(VerifyNumBlocks, 4);
		//PRINT;

	PROGRAM("test2 - correct capacities of blocks", 1024);
		CMD(Malloc, 32);
		CMD(Malloc, 256);
		CMD(Malloc, 21);
		CMD(Malloc, 32);
		CMD(VerifyBlockCapacities, { ROUND_UP(32,Alignment), ROUND_UP(256,Alignment), ROUND_UP(21,Alignment), ROUND_UP(32,Alignment) });
		CMD(Free, 1);
		CMD(VerifyBlockCapacities, { ROUND_UP(32,Alignment), ROUND_UP(256,Alignment), ROUND_UP(21,Alignment), ROUND_UP(32,Alignment) });
		CMD(Malloc, 6);
		CMD(VerifyBlockCapacities, { ROUND_UP(32,Alignment), ROUND_UP(6,Alignment), ROUND_UP(256,Alignment) - ROUND_UP(6,Alignment) - sizeof(BasicHeapBlockHeader), ROUND_UP(21,Alignment), ROUND_UP(32,Alignment) });
		CMD(Free, 2);
		CMD(VerifyBlockCapacities, { ROUND_UP(32,Alignment), ROUND_UP(6, Alignment), ROUND_UP(256,Alignment) - ROUND_UP(6, Alignment) - sizeof(BasicHeapBlockHeader) + ROUND_UP(21,Alignment), ROUND_UP(32,Alignment) });

	PROGRAM("test3 - correct sizes of blocks", 1024);
		CMD(Malloc, 32);
		CMD(Malloc, 256);
		CMD(Malloc, 21);
		CMD(Malloc, 32);
		CMD(VerifyBlockCurrentSizes, {32, 256, 21, 32});
		CMD(Free, 1);
		CMD(VerifyBlockCurrentSizes, {32, 0, 21, 32});
		CMD(Malloc, 6);
		CMD(VerifyBlockCurrentSizes, {32, 6, 0, 21, 32});
		CMD(Free, 2);
		CMD(VerifyBlockCurrentSizes, {32, 6, 0, 32});

	PROGRAM("test4 - Realloc, last block in chain, expands into empty heap", 1024);
		CMD(Malloc, 40);
		CMD(Malloc, 15);
		CMD(Malloc, 128);
		CMD(Malloc, 16);
		CMD(VerifyTopOfMemory, sizeof(BasicHeapBlockHeader) * 4 + ROUND_UP(40,Alignment) + ROUND_UP(15, Alignment) + ROUND_UP(128, Alignment) + ROUND_UP(16, Alignment));
		CMD(Realloc, ReallocData{ 3, 24 });
		CMD(VerifyTopOfMemory, sizeof(BasicHeapBlockHeader) * 4 + ROUND_UP(40, Alignment) + ROUND_UP(15, Alignment) + ROUND_UP(128, Alignment) + ROUND_UP(24, Alignment));
		CMD(VerifyNumBlocks, 4);
		CMD(VerifyBlockCapacities, { ROUND_UP(40,Alignment), ROUND_UP(15,Alignment), ROUND_UP(128,Alignment), ROUND_UP(24,Alignment) });
		CMD(VerifyBlockCurrentSizes, { 40, 15, 128, 24 });

}

int main()
{
	BuildProgramList();
	RunTestPrograms(gTestPrograms);
	return 0;
}