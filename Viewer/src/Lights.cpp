#include "Lights.h"


Light::Light()
{
	World.ResetTransformation();
	Local.ResetTransformation();
}

Light::~Light()
{
}

vec3 Light::GetPosition()
{
	return this->Light_Position;
}
float Light::GetRadiusLength()
{
	return this->Light_Radius;
}

void Light::SetPosition(vec3 input)
{
	this->Light_Position = input;
}

void Light::SetRadiusLength(float input)
{
	this->Light_Radius = input;
}
