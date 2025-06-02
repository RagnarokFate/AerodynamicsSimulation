#pragma once
#include "Scene.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"
#include "PostProcessor.h"

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
	
	// Post-processing controls
	void InitializePostProcessing(int width, int height);
	void ResizePostProcessing(int width, int height);
	void SetBlurParameters(float radius, float strength, int iterations);
	void EnableBlur(bool enable);
	bool IsBlurEnabled() const;

private:
	ShaderProgram ColorShader;
	
	// RGB Axis rendering resources
	GLuint axisVAO, axisVBO;
	bool axisBuffersInitialized = false;
	
	// Post-processing
	PostProcessor postProcessor;
	bool postProcessingInitialized = false;
	
};