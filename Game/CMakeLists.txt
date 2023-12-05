project(Game)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE INCS "include/*.h")
add_executable(Game ${SOURCES} ${INCS})

set_target_properties(Game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Game_SOURCE_DIR}/bin)

source_group(headers FILES ${INCS})

include_directories(${Game_SOURCE_DIR}/include)
target_include_directories(Game
	PRIVATE "../Engine/include" 
	PRIVATE "../vendor/glad/include"
	PRIVATE "../vendor/glfw/include"
	PRIVATE "../vendor/imgui"
	PRIVATE "../vendor/imgui/backends"
	PRIVATE "../vendor/glm/glm"
	)

target_link_libraries(Game
	PRIVATE Engine
	PRIVATE glad
	PRIVATE glfw
	PRIVATE ImGui
)

add_custom_command(TARGET Game POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	$<TARGET_FILE_DIR:Engine>/Engine.dll $<TARGET_FILE_DIR:Game>
)