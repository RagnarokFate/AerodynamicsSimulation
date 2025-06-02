#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <Transformation.h>
#include <MeshModel.h>

typedef enum Projection { Orthographic, Perspective, Frustum };
typedef enum View { Regular_Transformation, Look_At };

class Camera
{
public:
	Camera();
	virtual ~Camera();

	void Camera::SetCameraLookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up);
	void Camera::SetCameraOrtho(float Left_, float Up_, float Right_, float Down_);
	void Camera::SetCameraFrustum(float Left_, float Up_, float Right_, float Down_, float Near_, float Far_);
	void Camera::SetCameraPerspective(float FOV_, float Width_, float Height_, float Near_, float Far_);
	void Camera::SetViewTransformation();
	glm::mat4x4 GetProjectionTransformation();
	void Camera::SetMainMatrix();
	
	// FOV manipulation methods for zooming
	void Camera::AdjustFOV(float deltaFOV);
	void Camera::SetFOV(float newFOV);
	float Camera::GetFOV() const;
	


public:
	float Ratio = 1980 / 1080;
	int ProjectionStatus = 1; // Set to Perspective by default (1 = Perspective, 0 = Orthographic, 2 = Frustum)
	int ViewStatus = 0; // Restore original Regular_Transformation view mode
	public: 
	glm::mat4x4 view_transformation = mat4x4(1.0f);
	glm::mat4x4 projection_transformation = mat4x4(1.0f);
	glm::mat4x4 MainMatrix = mat4x4(1.0f);
	mat4x4 LookAt = mat4x4(1.0f);
	mat4x4 OrthoMatrix = mat4x4(1.0f);
	mat4x4 FrustumMatrix = mat4x4(1.0f);
	mat4x4 PerspectiveMatrix = mat4x4(1.0f);

	vec3 eye = vec3(0.0f, 0.0f, 0.0f); // Restore original camera position at origin
	vec3 at = vec3(0.0f, 0.0f, -1.0f); // Restore original look direction
	vec3 up = vec3(0.0f, 1.0f, 0.0f);

	float fov = 45.0f; // Default FOV of 45 degrees
	float nearPlane = 0.1f; // Restore original near plane
	float farPlane = 100.0f; // Restore original far plane

public:
	Transformation LocalTransformation = Transformation();
	Transformation WorldTransformation = Transformation();

};
