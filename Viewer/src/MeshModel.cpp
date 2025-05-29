#define _USE_MATH_DEFINES

#include "MeshModel.h"

MeshModel::MeshModel()
{
}

MeshModel::MeshModel(std::vector<Face> faces, std::vector<glm::vec3> vertices, std::vector<glm::vec3> normals, std::vector<glm::vec2> texturecoordinates, const std::string& model_name) :
	faces(faces),
	vertices(vertices),
	normals(normals),
	texturecoordinates(texturecoordinates),
	model_name(model_name)
{
	this->MeshModelTransformation.ResetTransformation();
	this->WorldTransformation.ResetTransformation();
	this->MeshModelTransformation.SetScaleBar(vec3(0.5f, 0.5f, 0.5f));

	vec3 MaxCoordinates = GetVertex(0, 0);
	vec3 MinCoordinates = GetVertex(0, 0);
	//Filling Vertex Dynamic Array With Vertices & Normals & Texture Coordinations
	VerticesData.reserve(3 * faces.size()); //Allocate VerticesData to 3x Faces->Size()
	for (int i = 0; i < faces.size(); i++)
	{
			Vertex vertex1;
			Vertex vertex2;
			Vertex vertex3;

			vertex1.Position = GetVertex(i, 0);
			vertex2.Position = GetVertex(i, 1);
			vertex3.Position = GetVertex(i, 2);

			if (normals.size())
			{
				vertex1.Normal = GetNormal(i, 0);
				vertex2.Normal = GetNormal(i, 1);
				vertex3.Normal = GetNormal(i, 2);
			}
			else
			{
				vec3 FaceNormal = triangleNormal(vertex1.Position, vertex2.Position, vertex3.Position);
				vertex1.Normal = FaceNormal;
				vertex2.Normal = FaceNormal;
				vertex3.Normal = FaceNormal;
			}

			if (texturecoordinates.size())
			{
				vertex1.TextureCoords = GetTextureCoordinates(i, 0);
				vertex2.TextureCoords = GetTextureCoordinates(i, 1);
				vertex3.TextureCoords = GetTextureCoordinates(i, 2);
			}
			else
			{
				vertex1.TextureCoords = vertex1.Position;
				vertex2.TextureCoords = vertex2.Position;
				vertex3.TextureCoords = vertex3.Position;
			}
			VerticesData.push_back(vertex1);
			VerticesData.push_back(vertex2);
			VerticesData.push_back(vertex3);
		
	}
	setupMesh();
	for (int i = 0; i < vertices.size(); i++)
	{
		MinCoordinates.x = std::min(MinCoordinates.x, vertices[i].x);
		MinCoordinates.y = std::min(MinCoordinates.y, vertices[i].y);
		MinCoordinates.z = std::min(MinCoordinates.z, vertices[i].z);

		MaxCoordinates.x = std::max(MaxCoordinates.x, vertices[i].x);
		MaxCoordinates.y = std::max(MaxCoordinates.y, vertices[i].y);
		MaxCoordinates.z = std::max(MaxCoordinates.z, vertices[i].z);

	}

	// Assign calculated bounds to public member variables
	MinPoints = MinCoordinates;
	MaxPoints = MaxCoordinates;
	CenterPoint = (MinCoordinates + MaxCoordinates) / 2.0f;
}

MeshModel::~MeshModel()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

}

const Face& MeshModel::GetFace(int index) const
{
	return faces[index];
}

int MeshModel::GetFacesCount() const
{
	return faces.size();
}

const std::string& MeshModel::GetModelName() const
{
	return model_name;
}

glm::vec3& MeshModel::GetVertex(int i, int j)
{
	return vertices[this->GetFace(i).GetVertexIndex(j) - float(1)];
}

glm::vec3& MeshModel::GetNormal(int i, int j)
{
	return normals[this->GetFace(i).GetNormalIndex(j) - float(1)];
}

glm::vec2& MeshModel::GetTextureCoordinates(int i, int j)
{
	return texturecoordinates[this->GetFace(i).GetTextureIndex(j) - float(1)];
}

void MeshModel::setupMesh()
{
	//unsigned int ~ GLuint
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, VerticesData.size() * sizeof(Vertex), &VerticesData[0], GL_STATIC_DRAW);

	// Vertex Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normals attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Vertex Texture Coords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// unbind to make sure other code does not change it somewhere else
	glBindVertexArray(0);
}

GLuint& MeshModel::GetVAO()
{
	return vao;
}

GLuint& MeshModel::GetVBO()
{
	return vbo;
}

void MeshModel::LoadTexture(const std::string& filePath)
{
	//texture.loadTexture(filePath, true);
	if (!texture.loadTexture(filePath, true))
	{
		texture.loadTexture(filePath, true);
	}
}

const std::vector<Vertex>& MeshModel::GetVerticesData()
{
	return VerticesData;
}

void MeshModel::UpdateModelVerticesData(std::vector<Vertex>& newVertices)
{
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, newVertices.size() * sizeof(Vertex), &newVertices[0]);

	glBindVertexArray(0);
}

void MeshModel::SetTextureMapping()
{
	std::vector<Vertex> newVertices(VerticesData);
	switch (this->MappingMode)
	{
	case Default:
		break;
	case PlaneMapping:
		for (Vertex& var : newVertices) {
			var.TextureCoords = vec2(var.Position.x, var.Position.z);
		}
		break;
	case CylinderMapping:
		for (Vertex& var : newVertices) {
			var.TextureCoords = vec2(0.5f + atan(var.Position.z, var.Position.x) / 2 * M_PI, 0.5f - asin(var.Position.y) / M_PI);

		}
		break;
	case SphereMapping:
		for (Vertex& var : newVertices) {
			float theta = atan(var.Position.z, var.Position.x) + M_PI;
			var.TextureCoords = vec2(theta, var.Position.y);
		}
		break;
	}

	UpdateModelVerticesData(newVertices);

}

void MeshModel::SetTextureMapping(int Type)
{
	std::vector<Vertex> newVertices(VerticesData);
	switch (Type)
	{
	case Default:
		break;
	case PlaneMapping:
		for (auto var : newVertices) {
			var.TextureCoords = vec2(var.Position.x, var.Position.z);
		}
		break;
	case CylinderMapping:
		for (auto var : newVertices) {
			var.TextureCoords = vec2(0.5f + atan(var.Position.z, var.Position.x) / 2 * M_PI, 0.5f - asin(var.Position.y) / M_PI);

		}
		break;
	case SphereMapping:
		for (auto var : newVertices) {
			float theta = atan(var.Position.z, var.Position.x) + M_PI;
			var.TextureCoords = vec2(theta, var.Position.y);
		}
		break;
	}

	UpdateModelVerticesData(newVertices);

}


//void MeshModel::SetTextureMapping(int Type)
//{
//	std::vector<Vertex> newVertices(VerticesData);
//	switch (Type)
//	{
//	case Default:
//		break;
//	case PlaneMapping:
//		for (auto var : newVertices) {
//			var.TextureCoords = vec2(var.Position.x + 0.5f, var.Position.z + 0.5f);
//			//var.TextureCoords = vec2(var.Position.x, var.Position.z);
//		}
//		break;
//	case CylinderMapping:
//		for (auto var : newVertices) {
//			var.TextureCoords = vec2(atan(var.Position.z, var.Position.x) / (2 * M_PI) + 0.5f, var.Position.y + 0.5f);
//			//var.TextureCoords = vec2(0.5f + atan(var.Position.z, var.Position.x) / 2 * M_PI, 0.5f - asin(var.Position.y) / M_PI);
//
//		}
//		break;
//	case SphereMapping:
//		for (auto var : newVertices) {
//			float phi = atan(var.Position.z, var.Position.x);
//			float theta = acos(var.Position.y);
//			var.TextureCoords = vec2(1.0f - phi / (2 * M_PI), theta / M_PI);
//		}
//		break;
//	default:
//		break;
//	}
//	UpdateModelVerticesData(newVertices);
//
//}