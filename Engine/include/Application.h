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

class APP_API Application
{
public:
	Application();
	~Application();

	void FreeCameraMovement(glm::mat4& transform, float deltaTime, float speed, const glm::vec3& worldUp = glm::vec3(0, 0, 1));

	void Run();

	void SetWindowSize(GLFWwindow* window, int width, int height);

private:
	void DebugCaptureVisibleTerrainNodes(DebugVisualizerTerrainOctree& terrainOctree);
	static void ProcessInput(GLFWwindow* window, float delta);
	static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
private:
	GLFWwindow* Window;

	int ScreenWidth;
	int ScreenHeight;

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

	
	std::vector<ITerrainOctreeNode*> VisibleNodes;
	
	
	static bool bWantMouseInput;
	static bool bWantKeyboardInput;
};

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}