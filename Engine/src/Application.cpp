#include "Application.h"

#include "Gizmos.h"

#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ext.hpp>
#include "TerrainOctree.h"
#include "DefaultAllocator.h"
bool Application::bWantMouseInput;
bool Application::bWantKeyboardInput;
Camera Application::DebugCamera;

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

	int screen_width, screen_height;
	glfwGetFramebufferSize(Window, &screen_width, &screen_height);
	glViewport(0, 0, screen_width, screen_height);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	

	ImGui_ImplGlfw_InitForOpenGL(Window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	ImGui::StyleColorsDark();

	// Initialise the Gizmos helper class
	Gizmos::Create();


	FOV = glm::pi<float>() * 0.25f;
	Aspect = (float) ScreenWidth/ (float)ScreenHeight;
	Near = 0.1f;
	Far = 3000.0f;
	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	ProjectionMatrix = glm::perspective(FOV, Aspect, Near, Far);

	CubeX = 0;
	CubeY = 0;
	CubeZ = 0;

	CubeOptionsOpen = false;

	glEnable(GL_DEPTH_TEST);
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

//void Application::FreeCameraMovement(glm::mat4& transform, float deltaTime, float speed, const glm::vec3& worldUp /*= glm::vec3(0, 1, 0)*/)
//{
//	Window = glfwGetCurrentContext();
//
//	glm::vec4 forward = transform[2];
//	glm::vec4 right = transform[0];
//	glm::vec4 up = transform[1];
//	glm::vec4 translation = transform[3];
//
//	float frameSpeed = glfwGetKey(Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? deltaTime * speed * 2 : deltaTime * speed;
//
//	// Translate camera
//	if (glfwGetKey(Window, 'W') == GLFW_PRESS)
//	{
//		translation -= forward * frameSpeed;
//	}
//	if (glfwGetKey(Window, 'S') == GLFW_PRESS)
//	{
//		translation += forward * frameSpeed;
//	}
//	if (glfwGetKey(Window, 'D') == GLFW_PRESS)
//	{
//		translation += right * frameSpeed;
//	}
//	if (glfwGetKey(Window, 'A') == GLFW_PRESS)
//	{
//		translation -= right * frameSpeed;
//	}
//	if (glfwGetKey(Window, 'Q') == GLFW_PRESS)
//	{
//		translation += up * frameSpeed;
//	}
//	if (glfwGetKey(Window, 'E') == GLFW_PRESS)
//	{
//		translation -= up * frameSpeed;
//	}
//	
//
//	transform[3] = translation;
//
//	// check for camera rotation
//	static bool mouseButtonDown = false;
//	if (glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
//	{
//		static double prevMouseX = 0;
//		static double prevMouseY = 0;
//
//		if (mouseButtonDown == false)
//		{
//			mouseButtonDown = true;
//			glfwGetCursorPos(Window, &prevMouseX, &prevMouseY);
//		}
//
//		double mouseX = 0, mouseY = 0;
//		glfwGetCursorPos(Window, &mouseX, &mouseY);
//
//		double deltaX = mouseX - prevMouseX;
//		double deltaY = mouseY - prevMouseY;
//
//		prevMouseX = mouseX;
//		prevMouseY = mouseY;
//
//		glm::mat4 mat;
//
//		// pitch
//		if (deltaY != 0)
//		{
//			mat = glm::axisAngleMatrix(right.xyz(), (float)-deltaY / 150.0f);
//			right = mat * right;
//			up = mat * up;
//			forward = mat * forward;
//		}
//
//		// yaw
//		if (deltaX != 0)
//		{
//			mat = glm::axisAngleMatrix(worldUp, (float)-deltaX / 150.0f);
//			right = mat * right;
//			up = mat * up;
//			forward = mat * forward;
//		}
//
//		transform[0] = right;
//		transform[1] = up;
//		transform[2] = forward;
//	}
//	else
//	{
//		mouseButtonDown = false;
//	}
//}

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
		DebugCamera.ProcessMouseMovement(dx, dy);
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
	TerrainVoxelVolume voxelVolume(&allocator);

	/*voxelVolume.SetValue({ 1,2,3 }, 28);
	i8 val = voxelVolume.GetValue({ 1,2,3 });
	std::cout << "Value: " << (int)val << "\n";*/
	DebugVisualizerTerrainOctree oct(&voxelVolume);
	SparseTerrainVoxelOctree sparse(&allocator, 2048, 50, -50);
	TerrainOctreeIndex octreeIndex = sparse.SetVoxelAt({ 100,200,300 }, 42);
	i8 valFromGet = sparse.GetVoxelAt({ 101,200,300 });
	SparseTerrainVoxelOctree::SparseTerrainOctreeNode* nodeFromIndex = sparse.FindNodeFromIndex(octreeIndex);
	//glfwSetCursorPosCallback(Window, Application::CursorPositionCallback);
	ImGuiIO& io = ImGui::GetIO();
	while (!glfwWindowShouldClose(Window))
	{
		bWantMouseInput = io.WantCaptureMouse;
		bWantKeyboardInput = io.WantCaptureKeyboard;

		glfwPollEvents();
		//FreeCameraMovement(DebugC, 0.0024, 300);
		ProcessInput(Window, 0.0024);


		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Gizmos::Clear();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (glfwGetKey(Window, 'J') == GLFW_PRESS)
		{
			DebugCaptureVisibleTerrainNodes(oct);
		}

		
		
		Gizmos::AddBox(
			glm::vec3(CubeX, CubeY, CubeZ),
			glm::vec3(1, 1, 1),
			true
		);

		//VoxelVolume.DebugVisualiseVolume();
		oct.DebugVisualiseChunks(VisibleNodes);
		

		Gizmos::Draw(DebugCamera.GetViewMatrix(), ProjectionMatrix);

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
			float inputVal = oct.GetMinimumViewportAreaThreshold();
			if(ImGui::InputFloat("MinimumViewportAreaThreshold", &inputVal))
			{
				oct.SetMinimumViewportAreaThreshold(inputVal);
			}
			ImGui::Checkbox("Fill Terrain Debug Boxes", &oct.bDebugFill);
			ImGui::InputInt("Mip level to draw", &oct.DebugMipLevelToDraw);
			
		}		
		ImGui::End();
		
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(Window);
	}
}

void Application::DebugCaptureVisibleTerrainNodes(DebugVisualizerTerrainOctree& terrainOctree)
{
	VisibleNodes.clear();
	terrainOctree.GetChunksToRender(DebugCamera, Aspect, FOV, Near, Far, VisibleNodes);
}
