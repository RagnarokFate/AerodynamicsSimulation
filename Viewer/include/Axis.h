#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Face.h"
#include <iostream>

using namespace glm;

class Axis
{
public:
	Axis();
	~Axis();

public:
	vec3 X_Local = vec3(1.0f,0.0f,0.0f);
	vec3 Y_Local = vec3(0.0f,1.0f,0.0f);
	vec3 Z_Local = vec3(0.0f,0.0f,1.0f);

};

