#include "CameraFunctionLibrary.h"
#include "Gizmos.h"
#include "Camera.h"

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

	//Gizmos::AddBox(translation, { 1.0,1.0,1.0 }, true);
	//Gizmos::AddSphere(translation, 10, 10, 0.5, { 1.0,1.0,0.0,1.0 });
	//Gizmos::AddLine(translation, translation + glm::normalize(forward) * 1.0f, { 1.0,1.0,1.0,1.0 });
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

Frustum CameraFunctionLibrary::CreateFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar)
{
	Frustum     frustum;
	const float halfVSide = zFar * tanf(fovY * .5f);
	const float halfHSide = halfVSide * aspect;
	const glm::vec3 frontMultFar = zFar * cam.Front;

	frustum.nearFace = { cam.Position + zNear * cam.Front, cam.Front };
	frustum.farFace = { cam.Position + frontMultFar, -cam.Front };
	frustum.rightFace = { cam.Position, glm::cross(cam.Up, frontMultFar + cam.Right * halfHSide) };
	frustum.leftFace = { cam.Position, glm::cross(frontMultFar - cam.Right * halfHSide, cam.Up) };
	frustum.topFace = { cam.Position, glm::cross(cam.Right, frontMultFar - cam.Up * halfVSide) };
	frustum.bottomFace = { cam.Position, glm::cross(frontMultFar + cam.Up * halfVSide, cam.Right) };

	return frustum;
}


