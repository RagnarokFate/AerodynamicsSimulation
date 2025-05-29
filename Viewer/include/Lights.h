#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Transformation.h"
#include <iostream>
#include <vector>

using namespace glm;
enum ShadingMode {Flat_Shading = 0,Gouraud_Shading = 1,Phong_Shading = 2};

class Light
{
public:

	friend Transformation;

	vec3 GetPosition();
	float GetRadiusLength();
	void SetPosition(vec3 input);
	void SetRadiusLength(float input);

	Light();
	~Light();

	//Parameters Declarations
public:
	vec3 AmbientColor = vec3(0.0f, 0.0f, 0.0f);
	vec3 DiffuseColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 SpecularColor = vec3(0.0f, 0.0f, 0.0f);
	float Specular_Degree = 96.0f;

	vec3 Light_Position = vec3(0.0f, 0.0f, 0.0f);
	float Light_Radius = 50.0f;
	vec3 Light_Color = vec3(1.0f, 1.0f, 1.0f);

	float LightIntensity = 1;
	vec3 LightSobel = vec3(1, 0, -1);
	
	Transformation& Local = Transformation();
	Transformation World = Transformation();

};

