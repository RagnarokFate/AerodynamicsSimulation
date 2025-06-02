#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "ShaderProgram.h"

class Skybox {
public:
  Skybox();
  ~Skybox();
  bool Load(const std::vector<std::string> &faces,
            const std::string &vertexShader,
            const std::string &fragmentShader);
  void Render(const glm::mat4 &view, const glm::mat4 &projection);

private:
  GLuint VAO = 0, VBO = 0, cubemapTexture = 0;
  ShaderProgram shader;
  void SetupMesh();
};