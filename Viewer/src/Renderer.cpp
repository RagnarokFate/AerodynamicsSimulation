#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cfloat>
#include "Renderer.h"
#include "InitShader.h"
#include "glm//gtx/normal.hpp"


#define INDEX(width,x,y,c) ((x)+(y)*(width))*3+(c)
#define Z_INDEX(width,x,y) ((x)+(y)*(width))

using namespace glm;
using namespace std;

//enum TextureMode { PlannerMapping, CylinderMapping, SphereMapping, NormalMapping, EnvironmentMapping, ToonShading};

Renderer::Renderer()
{
	// Constructor cleaned up - skybox and cubemap removed

}

Renderer::~Renderer()
{
	// Clean up axis rendering resources
	if (axisBuffersInitialized) {
		glDeleteVertexArrays(1, &axisVAO);
		glDeleteBuffers(1, &axisVBO);
	}
}

void Renderer::Render(const Scene& scene)
{
	scene.GetActiveCamera().SetMainMatrix();
	scene.GetActiveCamera().SetViewTransformation();

	mat4x4 CameraView = scene.GetActiveCamera().view_transformation;
	mat4x4 CameraProjection = scene.GetActiveCamera().GetProjectionTransformation();
	vec3 CameraPosition = scene.GetActiveCamera().eye;
	
	// Store mesh bounds for axis rendering
	vec3 globalMinBounds = vec3(FLT_MAX);
	vec3 globalMaxBounds = vec3(-FLT_MAX);
	bool hasMeshes = false;
	
	for (int i = 0; i < scene.GetModelCount() && !scene.GetLightCount() ;i++)
	{
		MeshModel& CurrentMesh = scene.GetModel(i);
		
		CurrentMesh.MeshModelTransformation.SetTransformation();
		CurrentMesh.WorldTransformation.SetTransformation();

		mat4x4 Local = CurrentMesh.MeshModelTransformation.GetMainMatrix();
		mat4x4 World = CurrentMesh.WorldTransformation.GetMainMatrix();
		DrawMesh(CurrentMesh, Local, World, CameraView, CameraProjection);
		
		// Update global bounds for axis rendering
		globalMinBounds = min(globalMinBounds, CurrentMesh.MinPoints);
		globalMaxBounds = max(globalMaxBounds, CurrentMesh.MaxPoints);
		hasMeshes = true;
	}
	
	if(scene.GetModelCount() && scene.GetLightCount())
	{
		DrawMesh(scene,CameraView,CameraProjection);
		
		// Calculate bounds from scene models for axis rendering
		for (int i = 0; i < scene.GetModelCount(); i++)
		{
			MeshModel& CurrentMesh = scene.GetModel(i);
			globalMinBounds = min(globalMinBounds, CurrentMesh.MinPoints);
			globalMaxBounds = max(globalMaxBounds, CurrentMesh.MaxPoints);
			hasMeshes = true;
		}
	}
	
	// Draw RGB coordinate axes at mesh edges if we have meshes
	if (hasMeshes) {
		DrawRGBAxes(globalMinBounds, globalMaxBounds, CameraView, CameraProjection);
	}
	
	// Skybox and Cubemap rendering removed for performance optimization
	
}

void Renderer::LoadShaders()
{
	ColorShader.loadShaders("vshader.glsl", "fshader.glsl");
	// Skybox and Cubemap shaders removed for performance optimization
	
	// Initialize axis rendering buffers
	InitializeAxisBuffers();
}

void Renderer::InitializeAxisBuffers()
{
	if (axisBuffersInitialized) return;
	
	// Generate axis VAO and VBO
	glGenVertexArrays(1, &axisVAO);
	glGenBuffers(1, &axisVBO);
	
	// We'll update the buffer data dynamically in DrawRGBAxes
	glBindVertexArray(axisVAO);
	glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
	
	// Position attribute (location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	// Color attribute (location 1) 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glBindVertexArray(0);
	axisBuffersInitialized = true;
}

void Renderer::DrawRGBAxes(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::mat4& view, const glm::mat4& projection)
{
	if (!axisBuffersInitialized) {
		InitializeAxisBuffers();
	}
	
	// Calculate axis length based on bounding box diagonal
	vec3 boundingSize = maxBounds - minBounds;
	float axisLength = length(boundingSize) * 0.15f; // 15% of diagonal for good visibility
	
	// Create axis lines at the edges of the bounding box
	// Each axis will be positioned at corners/edges for clear coordinate system indication
	
	// Start positions for each axis (at different corners)
	vec3 xAxisStart = vec3(minBounds.x, minBounds.y, minBounds.z);
	vec3 yAxisStart = vec3(minBounds.x, minBounds.y, minBounds.z);
	vec3 zAxisStart = vec3(minBounds.x, minBounds.y, minBounds.z);
	
	// End positions for each axis
	vec3 xAxisEnd = xAxisStart + vec3(axisLength, 0.0f, 0.0f);
	vec3 yAxisEnd = yAxisStart + vec3(0.0f, axisLength, 0.0f);
	vec3 zAxisEnd = zAxisStart + vec3(0.0f, 0.0f, axisLength);
	
	// RGB colors for X=Red, Y=Green, Z=Blue
	vec3 redColor = vec3(1.0f, 0.0f, 0.0f);
	vec3 greenColor = vec3(0.0f, 1.0f, 0.0f);
	vec3 blueColor = vec3(0.0f, 0.0f, 1.0f);
	
	// Vertex data: position + color (6 floats per vertex)
	float axisVertices[] = {
		// X-axis (Red)
		xAxisStart.x, xAxisStart.y, xAxisStart.z, redColor.r, redColor.g, redColor.b,
		xAxisEnd.x,   xAxisEnd.y,   xAxisEnd.z,   redColor.r, redColor.g, redColor.b,
		
		// Y-axis (Green)
		yAxisStart.x, yAxisStart.y, yAxisStart.z, greenColor.r, greenColor.g, greenColor.b,
		yAxisEnd.x,   yAxisEnd.y,   yAxisEnd.z,   greenColor.r, greenColor.g, greenColor.b,
		
		// Z-axis (Blue)
		zAxisStart.x, zAxisStart.y, zAxisStart.z, blueColor.r, blueColor.g, blueColor.b,
		zAxisEnd.x,   zAxisEnd.y,   zAxisEnd.z,   blueColor.r, blueColor.g, blueColor.b
	};
	
	// Update buffer data
	glBindVertexArray(axisVAO);
	glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_DYNAMIC_DRAW);
	
	// Use ColorShader for rendering
	ColorShader.use();
	ColorShader.setUniform("model", glm::mat4(1.0f)); // Identity matrix
	ColorShader.setUniform("view", view);
	ColorShader.setUniform("projection", projection);
	
	// Set line width for better visibility
	glLineWidth(3.0f);
	
	// Draw the axes as lines
	glDrawArrays(GL_LINES, 0, 6); // 6 vertices (3 lines of 2 vertices each)
	
	// Reset line width
	glLineWidth(1.0f);
	
	glBindVertexArray(0);
}

//void Renderer::DrawMesh(MeshModel& input, mat4x4 LocalTransformation = mat4x4(1.0f), mat4x4 WorldTransformation = mat4x4(1.0f), mat4x4 CameraView = mat4x4(1.0f), mat4x4 CameraProjection = mat4x4(1.0f))
void Renderer::DrawMesh(MeshModel& input, mat4x4 LocalTransformation, mat4x4 WorldTransformation, mat4x4 CameraView, mat4x4 CameraProjection)
{
	mat4x4 MeshModelTransfomration = WorldTransformation * LocalTransformation;
	mat4x4 Result = CameraProjection * CameraView * MeshModelTransfomration;
	input.UpdatedCenterPoint = vec3(Result * vec4(input.CenterPoint, 1.0f));

	{
		// Activate the 'colorShader' program (vertex and fragment shaders)
		ColorShader.use();
		vec3 EmptyVec = vec3(0.0f);
		// Set the uniform variables
		ColorShader.setUniform("model", MeshModelTransfomration);
		ColorShader.setUniform("view", CameraView);
		ColorShader.setUniform("projection", CameraProjection);
		//ColorShader.setUniform("material.textureMap", 0);
		ColorShader.setUniform("TriangleFillMode", input.TriangleFillMode);
		ColorShader.setUniform("MappingMode", input.MappingMode);
		ColorShader.setUniform("MappingExtra", input.MappingExtra);
		ColorShader.setUniform("MeshModelColor", input.MeshModelFillColor);

		ColorShader.setUniform("LightPosition", EmptyVec);
		ColorShader.setUniform("LightAmbient", EmptyVec);
		ColorShader.setUniform("LightDiffuse", EmptyVec);
		ColorShader.setUniform("LightSpecular", EmptyVec);
		ColorShader.setUniform("LightSpecularDegree", 0);
		ColorShader.setUniform("LightIntensity", 1);
		ColorShader.setUniform("LightSobel", EmptyVec);

		ColorShader.setUniform("MeshModelAmbient", EmptyVec);
		ColorShader.setUniform("MeshModelDiffuse", EmptyVec);
		ColorShader.setUniform("MeshModelSpecular", EmptyVec);

		ColorShader.setUniform("CameraPosition", EmptyVec);
		//input.texture.bind(0);

		if (input.TriangleFillMode == WithLightTexture || input.TriangleFillMode == WithTextureNoLight)
		{
			// Set 'texture1' as the active texture at slot #0
			input.texture.bind(0);
		}
		if (input.TriangleFill)
		{
			// Drag our model's faces (triangles) in fill mode with enhanced visibility
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBindVertexArray(input.GetVAO());
			glDrawArrays(GL_TRIANGLES, 0, input.GetVerticesData().size());
			glBindVertexArray(0);
		}
		
		// Enhanced wireframe visibility with thicker lines and brighter color
		ColorShader.setUniform("color", glm::vec3(0.8, 0.8, 0.8)); // Brighter wireframe
		glLineWidth(2.0f); // Thicker wireframe lines
		
		if (!input.OffGridDraw)
		{
			// Drag our model's faces (triangles) in line mode (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(input.GetVAO());
			glDrawArrays(GL_TRIANGLES, 0, input.GetVerticesData().size());
			glBindVertexArray(0);
		}
		
		// Reset line width
		glLineWidth(1.0f);
		if (input.TriangleFillMode == WithLightTexture || input.TriangleFillMode == WithTextureNoLight)
		{
			// Unset 'texture1' as the active texture at slot #0
			input.texture.unbind(0);
		}
		//input.texture.unbind(0);

	}
	
}

void Renderer::DrawMesh(Scene scene, mat4x4 CameraView, mat4x4 CameraProjection)
{
	for (int i = 0; i < scene.GetModelCount(); i++)
	{
		MeshModel& CurrentMesh = scene.GetModel(i);
		CurrentMesh.MeshModelTransformation.SetTransformation();
		CurrentMesh.WorldTransformation.SetTransformation();

		mat4x4 Result = CurrentMesh.WorldTransformation.GetMainMatrix() * CurrentMesh.MeshModelTransformation.GetMainMatrix();
		Light& LightSource = scene.GetActiveLight();
		LightSource.Local.SetTransformation();
		LightSource.World.SetTransformation();

		mat4x4 LightMatrix = LightSource.World.GetMainMatrix() * LightSource.Local.GetMainMatrix();
		vec3 UpdatedLightPosition = vec3(LightMatrix * vec4(LightSource.Light_Position, 1));
		{
			// Activate the 'colorShader' program (vertex and fragment shaders)
			ColorShader.use();

			// Set the uniform variables
			ColorShader.setUniform("model", Result);
			ColorShader.setUniform("view", CameraView);
			ColorShader.setUniform("projection", CameraProjection);
			ColorShader.setUniform("material.textureMap", 0);
			ColorShader.setUniform("TriangleFillMode", CurrentMesh.TriangleFillMode);
			ColorShader.setUniform("MappingMode", CurrentMesh.MappingMode);
			ColorShader.setUniform("MappingExtra", CurrentMesh.MappingExtra);
			ColorShader.setUniform("MeshModelColor", CurrentMesh.MeshModelFillColor);

			ColorShader.setUniform("LightPosition", UpdatedLightPosition);
			ColorShader.setUniform("LightAmbient", LightSource.AmbientColor);
			ColorShader.setUniform("LightDiffuse", LightSource.DiffuseColor);
			ColorShader.setUniform("LightSpecular", LightSource.SpecularColor);
			ColorShader.setUniform("LightSpecularDegree", LightSource.Specular_Degree);
			ColorShader.setUniform("LightIntensity", LightSource.LightIntensity);
			ColorShader.setUniform("LightSobel", LightSource.LightSobel);

			ColorShader.setUniform("MeshModelAmbient", CurrentMesh.AmbientColor);
			ColorShader.setUniform("MeshModelDiffuse", CurrentMesh.DiffuseColor);
			ColorShader.setUniform("MeshModelSpecular", CurrentMesh.SpecularColor);

			ColorShader.setUniform("CameraPosition", scene.GetActiveCamera().eye);

			if (CurrentMesh.TriangleFillMode == WithLightTexture || CurrentMesh.TriangleFillMode == WithTextureNoLight)
			{
				// Set 'texture1' as the active texture at slot #0
				CurrentMesh.texture.bind(0);
			}
			if (!CurrentMesh.OffGridDraw)
			{
				// Drag our model's faces (triangles) in line mode (wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glBindVertexArray(CurrentMesh.GetVAO());
				glDrawArrays(GL_TRIANGLES, 0, CurrentMesh.GetVerticesData().size());
				glBindVertexArray(0);
			}
			if (CurrentMesh.TriangleFill)
			{
				// Drag our model's faces (triangles) in fill mode
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glBindVertexArray(CurrentMesh.GetVAO());
				glDrawArrays(GL_TRIANGLES, 0, CurrentMesh.GetVerticesData().size());
				glBindVertexArray(0);
			}

			if (CurrentMesh.TriangleFillMode == WithLightTexture || CurrentMesh.TriangleFillMode == WithTextureNoLight)
			{
				// Unset 'texture1' as the active texture at slot #0
				CurrentMesh.texture.unbind(0);
			}
		}
		
	}

}

