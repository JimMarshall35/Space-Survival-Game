#pragma once
#include <glm.hpp>
#include <vector>

#include "Frustum.h"
class Camera;


class CameraFunctionLibrary
{
public:
	static void CreateFrustumFromCamera(const glm::mat4& camMatrix, float aspect, float fovY, float zNear, float zFar, Frustum& outFrustum);
	static bool IsSphereInFrustum(const glm::vec3& center, float radius, const Frustum& frustum);
	static bool IsSphereOnOrForwardPlane(const Plane& plane, const glm::vec3& center, float radius);
	static Frustum CreateFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar);
};


