#pragma once

#include "Core.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "TerrainVoxelVolume.h"
#include <vector>

class GLFWwindow;
class TerrainOctree;
struct TerrainOctreeNode;

class APP_API Application
{
public:
	Application();
	~Application();

	void FreeCameraMovement(glm::mat4& transform, float deltaTime, float speed, const glm::vec3& worldUp = glm::vec3(0, 0, 1));

	void Run();

protected:


private:
	void DebugCaptureVisibleTerrainNodes(TerrainOctree& terrainOctree);

private:
	GLFWwindow* Window;

	int ScreenWidth;
	int ScreenHeight;

	glm::mat4 CameraMatrix;
	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;

	bool CubeOptionsOpen;

	int CubeX = 0;
	int CubeY = 0;
	int CubeZ = 0;

	float FOV;
	float Aspect;
	float Near;
	float Far;

	
	std::vector<TerrainOctreeNode*> VisibleNodes;
	TerrainVoxelVolume VoxelVolume;
	
};