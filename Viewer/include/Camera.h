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
	


public:
	float Ratio = 1980 / 1080;
	int ProjectionStatus = 0;
	int ViewStatus = 0;
	public: 
	glm::mat4x4 view_transformation = mat4x4(1.0f);
	glm::mat4x4 projection_transformation = mat4x4(1.0f);
	glm::mat4x4 MainMatrix = mat4x4(1.0f);
	mat4x4 LookAt = mat4x4(1.0f);
	mat4x4 OrthoMatrix = mat4x4(1.0f);
	mat4x4 FrustumMatrix = mat4x4(1.0f);
	mat4x4 PerspectiveMatrix = mat4x4(1.0f);

	vec3 eye = vec3(0.0f, 0.0f, 0.0f);
	vec3 at = vec3(0.0f, 0.0f, -1.0f);
	vec3 up = vec3(0.0f, 1.0f, 0.0f);

	float fov = 1;

public:
	Transformation LocalTransformation = Transformation();
	Transformation WorldTransformation = Transformation();

};
