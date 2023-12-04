#include "pch.h"

int main(int argc, char** argv)
{
	// This allows us to call this executable with various command line arguments
	// which get parsed in InitGoogleTest
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
