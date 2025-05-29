#include "Transformation.h"


Transformation::Transformation()
{
	Scale = vec3(200.0f,200.0f,10.0f), Translate = vec3(640,360,1.0f), Rotate = vec3(0.0f,0.0f,0.0f);
}

Transformation::~Transformation()
{
}			

void Transformation::SetRotateMatrix()
{
	mat4x4 RotationX = mat4x4(1.0f), RotationY = mat4x4(1.0f), RotationZ = mat4x4(1.0f);

	RotationX[1][1] = cos(radians(this->Rotate.x));
	RotationX[2][1] = -sin(radians(this->Rotate.x));
	RotationX[1][2] = sin(radians(this->Rotate.x));
	RotationX[2][2] = cos(radians(this->Rotate.x));
	
	RotationY[0][0] = cos(radians(this->Rotate.y));
	RotationY[2][0] = -sin(radians(this->Rotate.y));
	RotationY[0][2] = sin(radians(this->Rotate.y));
	RotationY[2][2] = cos(radians(this->Rotate.y));

	RotationZ[0][0] = cos(radians(this->Rotate.z));
	RotationZ[1][0] = -sin(radians(this->Rotate.z));
	RotationZ[0][1] = sin(radians(this->Rotate.z));
	RotationZ[1][1] = cos(radians(this->Rotate.z));

	this->RotateMatrix = RotationX * RotationY * RotationZ;

}		

void Transformation::SetTranslateMatrix()
{
	this->TranslateMatrix[3][0] = this->Translate.x;
	this->TranslateMatrix[3][1] = this->Translate.y;
	this->TranslateMatrix[3][2] = this->Translate.z;
}	

void Transformation::SetScaleMatrix()
{
	this->ScaleMatrix[0][0] = this->Scale.x;
	this->ScaleMatrix[1][1] = this->Scale.y;
	this->ScaleMatrix[2][2] = this->Scale.z;
}      

float Transformation::Getx() {
	return Translate.x;
}

float Transformation::Gety() {
	return Translate.y;
}

float Transformation::Getz() {
	return Translate.z;
}

void Transformation::SetTransformation()
{
	this->SetRotateMatrix();
	this->SetScaleMatrix();
	this->SetTranslateMatrix();

	this->MainMatrix = this->TranslateMatrix * this->ScaleMatrix * this->RotateMatrix;
}

void Transformation::SetTranslateBar(vec3 input)
{
	this->Translate = input;
}

void Transformation::SetScaleBar(vec3 input)
{
	this->Scale = input;
}

void Transformation::SetRotateBar(vec3 input)
{
	this->Rotate = input;
}

vec3 Transformation::GetTranslateBar()
{
	return this->Translate;
}

vec3 Transformation::GetScaleBar()
{
	return this->Scale;
}

vec3 Transformation::GetRotateBar()
{
	return this->Rotate;
}

mat4x4 Transformation::GetMainMatrix()
{
	return this->MainMatrix;
}

void Transformation::ResetTransformation()
{
	this->Scale = vec3(1.0f,1.0f,1.0f);
	this->Translate = vec3(0.0f,0.0f,0.0f);
	this->Rotate = vec3(0.0f,0.0f,0.0f);

	this->RotateMatrix = mat4x4(1.0f);
	this->ScaleMatrix = mat4x4(1.0f);
	this->TranslateMatrix = mat4x4(1.0f);

	this->MainMatrix = mat4x4(1.0f);
}