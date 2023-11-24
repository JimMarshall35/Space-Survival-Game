#include "Application.h"

#include "Gizmos.h"

#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ext.hpp>
#include "DebugVisualizerTerrainOctree.h"
#include "SparseTerrainVoxelOctree.h"
#include "DefaultAllocator.h"
#include "TerrainPolygonizer.h"
#include "TerrainRenderer.h"
#include "TerrainMaterial.h"
#include "TerrainLight.h"
#include "TestProceduralTerrainVoxelPopulator.h"
#include "ThreadPool.h"
#include <memory>

bool Application::bWantMouseInput;
bool Application::bWantKeyboardInput;
bool Application::bWireframeMode;
bool Application::bDebugDrawChunks;
bool Application::bDebugVoxels;
glm::ivec3 Application::SelectedVoxel;
float Application::CursorDistance;

Camera Application::DebugCamera;

static SparseTerrainVoxelOctree* sOctree;

static void GLAPIENTRY MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

Application::Application()
{
	
	// Setup Window
	if (!glfwInit())
		return;

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	ScreenWidth = 1280;
	ScreenHeight = 720;

	// Create Window with graphics context
	Window = glfwCreateWindow(ScreenWidth, ScreenHeight, "SPACE GAME", NULL, NULL);
	if (Window == NULL)
		return;

	glfwMakeContextCurrent(Window);
	glfwSwapInterval(1); // Enable vsync

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw("unable to context to OpenGL");

	glfwSetWindowUserPointer(Window, this);
	glfwSetFramebufferSizeCallback(Window, [](GLFWwindow* window, int width, int height)
		{
			if (Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window)))
			{
				app->SetWindowSize(window, width, height);
			}
		});
	SetWindowSize(Window, ScreenWidth, ScreenHeight);
	glfwSetScrollCallback(Window, &ScrollCallback);
	glDebugMessageCallback(MessageCallback, 0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	

	ImGui_ImplGlfw_InitForOpenGL(Window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	ImGui::StyleColorsDark();

	// Initialise the Gizmos helper class
	Gizmos::Create();

	CubeX = 0;
	CubeY = 0;
	CubeZ = 0;

	CubeOptionsOpen = false;

	glEnable(GL_DEPTH_TEST);
	bDebugVoxels = false;
	bDebugDrawChunks = false;
	SelectedVoxel = {0,0,0};
	CursorDistance = 3.0f;
}

Application::~Application()
{
	Gizmos::Destroy();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(Window);
	glfwTerminate();
}



void Application::ProcessInput(GLFWwindow* window, float delta)
{
	if (bWantKeyboardInput)
	{
		return;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	CursorPositionCallback(window, mouseX, mouseY);

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		DebugCamera.ProcessKeyboard(FORWARD, delta);
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		DebugCamera.ProcessKeyboard(BACKWARD, delta);
	}
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		DebugCamera.ProcessKeyboard(LEFT, delta);
	}
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		DebugCamera.ProcessKeyboard(RIGHT, delta);
	}
}

void Application::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (bWantMouseInput)
	{
		return;
	}

	static double lastx = 0;
	static double lasty = 0;

	double dx = xpos - lastx;
	double dy = ypos - lasty;

	lastx = xpos;
	lasty = ypos;
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS) {
		DebugCamera.ProcessMouseMovement(dx, -dy);
	}
}

struct dummy
{
	u16 di;
	i8 da;
};
void Application::Run()
{
	DefaultAllocator allocator;

	/*voxelVolume.SetValue({ 1,2,3 }, 28);
	i8 val = voxelVolume.GetValue({ 1,2,3 });
	std::cout << "Value: " << (int)val << "\n";*/
	TerrainRenderer renderer({
		100000000,
		GPUMemoryFreeingStrategy::OvershootFixedAmount,
		10000
		});

	TerrainMaterial material;
	material.AmbientStrength = 1.0;
	material.DiffuseStrength = 1.0;
	material.Colour = { 0.0, 1.0, 0.0 };
	material.Shinyness = 16;
	material.SpecularStrength = 1.0;

	TerrainLight light;
	light.LightColor = { 1.0,1.0,1.0 };
	light.LightPosition = { 1024, 1024, 2048 };

	renderer.SetTerrainLight(light);
	renderer.SetTerrainMaterial(material);

	std::shared_ptr<rdx::thread_pool> threadPool = std::make_shared<rdx::thread_pool>(std::thread::hardware_concurrency());

	TerrainPolygonizer polygonizer(&allocator, threadPool);
	TestProceduralTerrainVoxelPopulator pop(threadPool);
	SparseTerrainVoxelOctree sparse(&allocator, &polygonizer, &renderer, 2048, 50, -50, &pop);
	sOctree = &sparse;

	ImGuiIO& io = ImGui::GetIO();
	std::vector<ITerrainOctreeNode*> outNodes;
	glm::mat4 identity(1.0f);

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	while (!glfwWindowShouldClose(Window))
	{
		bWantMouseInput = io.WantCaptureMouse;
		bWantKeyboardInput = io.WantCaptureKeyboard;

		glfwPollEvents();
		//FreeCameraMovement(DebugC, 0.0024, 300);
		ProcessInput(Window, 0.0024);
		outNodes.clear();
		sparse.GetChunksToRender(DebugCamera, Aspect, FOV, Near, Far, outNodes);


		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Gizmos::Clear();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		
		Gizmos::AddBox(
			glm::vec3(CubeX, CubeY, CubeZ),
			glm::vec3(1, 1, 1),
			true
		);

		//VoxelVolume.DebugVisualiseVolume();
		//oct.DebugVisualiseChunks(VisibleNodes);

		renderer.RenderTerrainNodes(outNodes, identity, DebugCamera.GetViewMatrix(), ProjectionMatrix, bDebugDrawChunks);
		

		ImGui::SetNextItemWidth(200);
		if (ImGui::Begin("Objects", &CubeOptionsOpen, 0))
		{
			if (ImGui::TreeNode("Cube"))
			{
				ImGui::DragInt("Position X", &CubeX, 1.0f, -100, 100);
				ImGui::DragInt("Position Y", &CubeY, 1.0f, -100, 100);
				ImGui::DragInt("Position Z", &CubeZ, 1.0f, -100, 100);

				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Terrain"))
			{
				if (ImGui::Checkbox("WireFrame", &bWireframeMode))
				{
					if (bWireframeMode)
					{
						glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
					}
					else
					{
						glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
					}
				}
				ImGui::Checkbox("VisualiseTerrainChunks", &bDebugDrawChunks);
				ImGui::Checkbox("DebugVoxels", &bDebugVoxels);
				if (bDebugVoxels)
				{
					DrawBoxAroundSelectedVoxel();
					size_t octreeSize = sparse.GetSize();
					if (SelectedVoxel.x >= 0 && SelectedVoxel.x < octreeSize &&
						SelectedVoxel.y >= 0 && SelectedVoxel.y < octreeSize &&
						SelectedVoxel.z >= 0 && SelectedVoxel.z < octreeSize)
					{
						ImGuiPrintSelectedVoxelInfo(sparse);
					}
				}
				ImGui::SliderFloat("Cursor distance", &CursorDistance, 3.0f, 100.0f);
				ImGui::TreePop();
			}
		}		
		ImGui::End();
		
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		Gizmos::Draw(DebugCamera.GetViewMatrix(), ProjectionMatrix);

		glfwSwapBuffers(Window);
	}
}

void Application::SetWindowSize(GLFWwindow* window, int width, int height)
{
	glfwGetFramebufferSize(Window, &ScreenWidth, &ScreenHeight);
	glViewport(0, 0, ScreenWidth, ScreenHeight);

	FOV = glm::pi<float>() * 0.25f;
	Aspect = (float)ScreenWidth / (float)ScreenHeight;
	Near = 0.1f;
	Far = 5000.0f;
	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	ProjectionMatrix = glm::perspective(FOV, Aspect, Near, Far);
}

//void Application::DebugCaptureVisibleTerrainNodes(DebugVisualizerTerrainOctree& terrainOctree)
//{
//	VisibleNodes.clear();
//	terrainOctree.GetChunksToRender(DebugCamera, Aspect, FOV, Near, Far, VisibleNodes);
//}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void Application::DrawBoxAroundSelectedVoxel()
{
	using namespace glm;
	vec3 cursorPos = DebugCamera.Position + normalize(DebugCamera.Front) * CursorDistance;
	SelectedVoxel = {
		floor(cursorPos.x),
		floor(cursorPos.y),
		floor(cursorPos.z)
	};
	Gizmos::AddBox(glm::vec3(SelectedVoxel) + vec3{0.5f,0.5f,0.5f},
		vec3{1,1,1},
		false);

}

void Application::ImGuiPrintSelectedVoxelInfo(SparseTerrainVoxelOctree& octree)
{
	ImGui::Text("Selected Voxel Value: %i", octree.GetVoxelAt(SelectedVoxel));
}
void Application::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	i8 val = sOctree->GetVoxelAt(SelectedVoxel);
	sOctree->SetVoxelAt(SelectedVoxel, yoffset < 0 ? val - 1 : val + 1);
}