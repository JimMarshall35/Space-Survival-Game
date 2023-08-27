#pragma once
#include <glm.hpp>

struct Plane
{
	glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
	float     distance = 0.f;        // Distance with origin

	Plane() = default;

	Plane(const glm::vec3& p1, const glm::vec3& norm)
		: normal(glm::normalize(norm)),
		distance(glm::dot(normal, p1))
	{}

	float getSignedDistanceToPlane(const glm::vec3& point) const
	{
		return glm::dot(normal, point) - distance;
	}
};

struct Frustum
{
	Plane topFace;
	Plane bottomFace;

	Plane rightFace;
	Plane leftFace;

	Plane farFace;
	Plane nearFace;
};


class CameraFunctionLibrary
{
public:
	static void CreateFrustumFromCamera(const glm::mat4& camMatrix, float aspect, float fovY, float zNear, float zFar, Frustum& outFrustum);
	static bool IsSphereInFrustum(const glm::vec3& center, float radius, const Frustum& frustum);
	static bool IsSphereOnOrForwardPlane(const Plane& plane, const glm::vec3& center, float radius);
};