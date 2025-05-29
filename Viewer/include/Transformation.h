#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <imgui/imgui.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <glm/glm.hpp>

using namespace glm;

class Transformation
{
private:
	vec3 Scale;
	vec3 Translate;
	vec3 Rotate;

public :mat4x4 ScaleMatrix = mat4x4(1.0f), TranslateMatrix = mat4x4(1.0f), RotateMatrix = mat4x4(1.0f);
	mat4x4 MainMatrix = mat4x4(1.0f);
public:
	Transformation();
	~Transformation();
	
	void SetRotateMatrix();
	void SetTranslateMatrix();
	void SetScaleMatrix();
	void SetTransformation();

	vec3 Transformation::GetTranslateBar();
	vec3 Transformation::GetScaleBar();
	vec3 Transformation::GetRotateBar();
	float Transformation::Getx();
	float Transformation::Gety();
	float Transformation::Getz();

	void Transformation::SetTranslateBar(vec3 input);
	void Transformation::SetScaleBar(vec3 input);
	void Transformation::SetRotateBar(vec3 input);

	mat4x4 Transformation::GetMainMatrix();

	void Transformation::ResetTransformation();


};


