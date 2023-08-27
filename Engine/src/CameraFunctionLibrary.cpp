#include "CameraFunctionLibrary.h"

void CameraFunctionLibrary::CreateFrustumFromCamera(const glm::mat4& camMatrix, float aspect, float fovY, float zNear, float zFar, Frustum& outFrustum)
{
	const float halfVSide = zFar * tanf(fovY * .5f);
	const float halfHSide = halfVSide * aspect;

	glm::vec3 forward = camMatrix[2];
	glm::vec3 right = camMatrix[0];
	glm::vec3 up = camMatrix[1];
	glm::vec3 translation = camMatrix[3];

	const glm::vec3 frontMultFar = zFar * forward;

	outFrustum.nearFace = { translation + zNear * forward, forward };
	outFrustum.farFace = { translation + frontMultFar, -forward };
	outFrustum.rightFace = { translation, glm::cross(up, frontMultFar + right * halfHSide) };
	outFrustum.leftFace = { translation, glm::cross(frontMultFar - right * halfHSide, up) };
	outFrustum.topFace = { translation, glm::cross(right, frontMultFar - up * halfVSide) };
	outFrustum.bottomFace = { translation, glm::cross(frontMultFar + up * halfVSide, right) };
}

bool CameraFunctionLibrary::IsSphereInFrustum(const glm::vec3& center, float radius, const Frustum& frustum)
{
	return 
		IsSphereOnOrForwardPlane(frustum.leftFace, center, radius) &&
		IsSphereOnOrForwardPlane(frustum.rightFace, center, radius) &&
		IsSphereOnOrForwardPlane(frustum.topFace, center, radius) &&
		IsSphereOnOrForwardPlane(frustum.bottomFace, center, radius) &&
		IsSphereOnOrForwardPlane(frustum.nearFace, center, radius) &&
		IsSphereOnOrForwardPlane(frustum.farFace, center, radius);
}

bool CameraFunctionLibrary::IsSphereOnOrForwardPlane(const Plane& plane, const glm::vec3& center, float radius)
{
	return plane.getSignedDistanceToPlane(center) > -radius;
}
