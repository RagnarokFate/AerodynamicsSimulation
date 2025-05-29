#include "Camera.h"

Camera::Camera()
{
	//Reset Transformation Local/World Matrices:
	mat4x4 IdentityMatrix = mat4x4(1.0f);
	this->LocalTransformation.ResetTransformation();
	this->WorldTransformation.ResetTransformation();
	this->LocalTransformation.MainMatrix = IdentityMatrix;
	this->WorldTransformation.MainMatrix = IdentityMatrix;
	view_transformation = IdentityMatrix;
	projection_transformation = IdentityMatrix;

}

Camera::~Camera()
{
	
}

glm::mat4x4 Camera::GetProjectionTransformation()
{
	if (this->ProjectionStatus == Orthographic)
		projection_transformation = this->OrthoMatrix;
	if (this->ProjectionStatus == Perspective)
		projection_transformation = this->PerspectiveMatrix;
	if (this->ProjectionStatus == Frustum)
		projection_transformation = this->FrustumMatrix;
		

	return projection_transformation;
}

void Camera::SetViewTransformation()
{
	if (ViewStatus == Regular_Transformation)
	{
		this->LocalTransformation.SetTransformation();
		this->WorldTransformation.SetTransformation();
		view_transformation = inverse(this->WorldTransformation.MainMatrix * this->LocalTransformation.MainMatrix);
	}
	if (ViewStatus == Look_At)
	{
		view_transformation = inverse(LookAt);
	}
}

void Camera::SetCameraLookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up)
{
	LookAt = glm::lookAt(eye, at, up);
}

void Camera::SetCameraOrtho(float Left_, float Up_, float Right_, float Down_)
{
	OrthoMatrix = glm::ortho(Left_, Right_, Down_, Up_);
}

void Camera::SetCameraFrustum(float Left_, float Up_, float Right_, float Down_ ,float Near_ , float Far_)
{
	FrustumMatrix = glm::frustum(Left_, Right_, Down_, Up_,Near_,Far_);
	
}

void Camera::SetCameraPerspective(float FOV_, float Width_, float Height_, float Near_, float Far_)
{
	PerspectiveMatrix = glm::perspective(radians(FOV_), Width_/ Height_, Near_, Far_);
}

void Camera::SetMainMatrix()
{
	MainMatrix = view_transformation * GetProjectionTransformation();
}