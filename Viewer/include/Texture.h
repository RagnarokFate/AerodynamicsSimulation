#pragma once
#ifndef Texture_H
#define Texture_H

#include <glad/glad.h>
#include <string>
#include<iostream>
#include<iterator> // for iterators
#include<vector> // for vectors

using namespace std;
using std::string;

class Texture
{
public:
	Texture();
	virtual ~Texture();

	bool loadTexture(const string& fileName, bool generateMipMaps = true);
	void bind(GLuint texUnit = 0);
	void unbind(GLuint texUnit = 0);
	unsigned int loadCubemap(std::vector<std::string> faces);

private:
	Texture(const Texture& rhs) {}
	Texture& operator = (const Texture& rhs) {}

	GLuint mTexture;
};
#endif
