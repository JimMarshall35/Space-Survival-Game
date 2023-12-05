project(Engine)
add_compile_definitions(BUILD_DLL)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE INCS "include/*.h")
add_library(Engine SHARED ${SOURCES} ${INCS})

set_target_properties(Engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Engine_SOURCE_DIR}/bin)

source_group(headers FILES ${INCS})

include_directories(${Engine_SOURCE_DIR}/include)
target_include_directories(Engine 
	PUBLIC "../vendor/glad/include"
	PUBLIC "../vendor/glfw/include"
	PUBLIC "../vendor/imgui"
	PUBLIC "../vendor/imgui/backends"
	PUBLIC "../vendor/glm/glm"
	)
target_link_libraries(Engine
	PRIVATE opengl32
	PRIVATE glad
	PRIVATE glfw
	PRIVATE ImGui
)