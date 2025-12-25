#pragma once
#include "glad/glad.h"
#include "GLFW/glfw3.h"

class Texture
{
private: 
	unsigned int tb;
	unsigned int tex;
public:
	Texture(const void* data, int size);
	~Texture();

	void Bind() const;
	void UnBind() const;
};