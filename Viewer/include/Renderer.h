#pragma once
#include "Scene.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"

class Renderer
{
public:
	Renderer();	
	virtual ~Renderer();
	void Render(const Scene& scene);
	void LoadShaders();
	void Renderer::DrawMesh(MeshModel& input, mat4x4 LocalTransformation, mat4x4 WorldTransformation, mat4x4 CameraView , mat4x4 CameraProjection);
	void Renderer::DrawMesh(Scene scene, mat4x4 CameraView, mat4x4 CameraProjection);
	
	// RGB Axis rendering methods
	void DrawRGBAxes(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::mat4& view, const glm::mat4& projection);
	void InitializeAxisBuffers();

private:
	ShaderProgram ColorShader;
	
	// RGB Axis rendering resources
	GLuint axisVAO, axisVBO;
	bool axisBuffersInitialized = false;
	
};