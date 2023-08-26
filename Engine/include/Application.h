#pragma once

#include "Core.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>

class GLFWwindow;

class APP_API Application
{
public:
	Application();
	~Application();

	void FreeCameraMovement(glm::mat4& transform, float deltaTime, float speed, const glm::vec3& worldUp = glm::vec3(0, 1, 0));

	void Run();

protected:


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
};