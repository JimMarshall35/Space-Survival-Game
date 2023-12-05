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
	PUBLIC "../vendor/tinyxml2"
	)
target_link_libraries(Engine
	PRIVATE opengl32
	PRIVATE glad
	PRIVATE glfw
	PRIVATE ImGui
	PRIVATE tinyxml2
)

add_custom_command(TARGET Engine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:Engine>/Engine.dll ${Editor_SOURCE_DIR}/bin/$<CONFIGURATION>
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:Engine>/Engine.dll ${Game_SOURCE_DIR}/bin/$<CONFIGURATION>
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:Engine>/Engine.dll ${AllocatorTest_SOURCE_DIR}/bin/$<CONFIGURATION>
)