add_subdirectory(glad)
add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(googletest)

file(GLOB IMGUI_BACKEND
	"imgui/backends/imgui_impl_glfw.h"
	"imgui/backends/imgui_impl_glfw.cpp"
	"imgui/backends/imgui_impl_opengl3.h"
	"imgui/backends/imgui_impl_opengl3.cpp"
)
file(GLOB IMGUI_SOURCES "imgui/*.h" "imgui/*.cpp")
add_library(ImGui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKEND})
target_include_directories(ImGui
	PUBLIC "imgui"
	PUBLIC "glfw/include"
	)