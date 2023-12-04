#pragma once

#include "Core.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <vector>
#include "Camera.h"

class GLFWwindow;
class DebugVisualizerTerrainOctree;
struct ITerrainOctreeNode;
struct ImGuiIO;
class SparseTerrainVoxelOctree;

class APP_API Application
{
public:
	Application();
	~Application();

	void FreeCameraMovement(glm::mat4& transform, float deltaTime, float speed, const glm::vec3& worldUp = glm::vec3(0, 0, 1));

	void DrawGrid();

	void Run();

	void SetWindowSize(GLFWwindow* window, int width, int height);

private:
	//void DebugCaptureVisibleTerrainNodes(DebugVisualizerTerrainOctree& terrainOctree);
	static void ProcessInput(GLFWwindow* window, float delta);
	static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
	static void DrawBoxAroundSelectedVoxel();
	static void ImGuiPrintSelectedVoxelInfo(SparseTerrainVoxelOctree& octree);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);


private:
	static void FrameBufferSizeChangedCallback(GLFWwindow* window, int width, int height);

private:

	GLFWwindow* Window;

	static int ScreenWidth;
	static int ScreenHeight;

	static Camera DebugCamera;
	glm::mat4 ProjectionMatrix;


	bool CubeOptionsOpen;

	int CubeX = 0;
	int CubeY = 0;
	int CubeZ = 0;

	float FOV;
	float Aspect;
	float Near;
	float Far;

	float MaxCameraSpeed = 2000.0f;
	float MinCameraSpeed = 0.0f;
	float CameraSpeed = 1000.0f;

	
	std::vector<ITerrainOctreeNode*> VisibleNodes;
	
	
	static bool bWantMouseInput;
	static bool bWantKeyboardInput;
	static bool bWireframeMode;
	static bool bDebugDrawChunks;
	static bool bDebugVoxels;
	static float CursorDistance;

	static glm::ivec3 SelectedVoxel;
};

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
