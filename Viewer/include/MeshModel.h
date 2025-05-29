#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Face.h"
#include "Transformation.h"
#include <iostream>
#include <Axis.h>
#include <vector>
#include "Texture.h"
#include "glm//gtx/normal.hpp"

using namespace std;
typedef enum MappingTypes { Default, PlaneMapping, CylinderMapping, SphereMapping };
typedef enum MappingExtra { NoMapping ,NormalMapping, EnvironmentMapping, ToonShading};
typedef enum FillMode { Normal, SpecficColor, WithLightNoTexture, WithTextureNoLight, WithLightTexture };

struct Vertex
{
	vec3 Position = vec3(0.0f, 0.0f, 0.0f);
	vec3 Normal = vec3(0.0f, 0.0f, 0.0f);
	vec2 TextureCoords = vec2(0.0f,0.0f);
};


class MeshModel
{
public:
	MeshModel::MeshModel();
	MeshModel::MeshModel(std::vector<Face> faces, std::vector<glm::vec3> vertices, std::vector<glm::vec3> normals, std::vector<glm::vec2> texturecoordinates, const std::string& model_name);

	virtual ~MeshModel();
	const Face& GetFace(int index) const;
	int GetFacesCount() const;
	const std::string& GetModelName() const;
	glm::vec3& MeshModel::GetVertex(int i, int j);
	glm::vec3& MeshModel::GetNormal(int i, int j);
	glm::vec2& MeshModel::GetTextureCoordinates(int i, int j);
	void setupMesh();
	GLuint& MeshModel::GetVAO();
	GLuint& MeshModel::GetVBO();
	void MeshModel::LoadTexture(const std::string& filePath);
	const std::vector<Vertex>& MeshModel::GetVerticesData();
	void MeshModel::UpdateModelVerticesData(std::vector<Vertex>& newVertices);
	void MeshModel::SetTextureMapping();
	void MeshModel::SetTextureMapping(int Type);
private:
	std::vector<Face> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texturecoordinates;
	std::string model_name;

public :
	vec3 MeshModelGridColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 MeshModelFillColor = vec3(1.0f, 0.0f, 0.0f);
	
	vec3 NormalsColor = vec3(1.0f,1.0f,1.0f);
	vec3 CenterOfFaceColor = vec3(1.0f,1.0f,1.0f);
	vec3 BoundingBoxColor = vec3(0.0f, 1.0f, 0.0f);
	vec3 WorldBoundingBoxColor = vec3(0.8f, 0.8f, 0.3f);
	vec3 FacesRectangleColor = vec3(1.0f, 1.0f, 1.0f);

	bool NormalsDraw = false, CenterOfFaceDraw = false, BoundingBoxDraw = false, OffGridDraw = false, Local_Axis = false, WorldBoundingBox = false, DrawFacesRectangles = false, FacesRectangles = false, TriangleFill = false;

	float NormalsLength = 50.0f;
	float CenterOfFaceLength = 50.0f;

	Transformation MeshModelTransformation = Transformation();
	Transformation WorldTransformation = Transformation();
	Axis MeshModelAxis = Axis();
	float AxisLength = 50.0f;

	int TriangleFillMode = 0;
	int ScanLineMode = 0;
	int WalkingEdgeMode = 0;

public:
	vec3 MinPoints;
	vec3 MaxPoints;
	vec3 CenterPoint = vec3(0.0f,0.0f,0.0f);
	vec3 UpdatedCenterPoint = vec3(0.0f,0.0f,0.0f);
	vec3 EdgePoints[8];
	
public:
	vec3 AmbientColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 DiffuseColor = vec3(1.0f, 1.0f, 1.0f);
	vec3 SpecularColor = vec3(1.0f, 1.0f, 1.0f);

	//Assignment OpenGL
public:
	vector<Vertex> VerticesData;
	GLuint vbo;
	GLuint vao;
	int MappingMode = Default;
	int MappingExtra = NoMapping;
	Texture texture;

};
