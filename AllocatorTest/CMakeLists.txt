project(AllocatorTest)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE INCS "include/*.h")
add_executable(AllocatorTest ${SOURCES} ${INCS})

set_target_properties(AllocatorTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${AllocatorTest_SOURCE_DIR}/bin)

source_group(headers FILES ${INCS})

include_directories(${AllocatorTest_SOURCE_DIR}/include)
target_include_directories(AllocatorTest
	PRIVATE "../Engine/include" 
	PRIVATE "../vendor/glad/include"
	PRIVATE "../vendor/glfw/include"
	PRIVATE "../vendor/imgui"
	PRIVATE "../vendor/imgui/backends"
	PRIVATE "../vendor/glm/glm"
	)

target_link_libraries(AllocatorTest
	PRIVATE Engine
	PRIVATE glad
	PRIVATE glfw
	PRIVATE ImGui
)