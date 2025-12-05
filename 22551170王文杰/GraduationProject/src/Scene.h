#pragma once

#include "Shape.h"
#include "BVH.h"

#include <vector>

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;



class Scene {
private:
	std::vector<shape*> models;
	std::vector<Light> Lights;

	std::vector<float> vertices;
	std::vector<unsigned int> indices;

	std::vector<Triangle_encoded> trianglesData;
	std::vector<BVHNode_encode> BVHData;
	
	// 三角形数据
	GLuint tbo, texBuffer;
	
	// BVH数据
	GLuint bvh_tbo, bvh_texBuffer;

	GLuint VAO, VBO, EBO;
	bool inited, modified;
public:
	Scene();
	Scene(std::vector<shape*> model);
	~Scene();

	void push(shape* s);
	void push(Light light);
	
	void Render(Shader& shader);
	void Draw(Shader& shader);
	void BindTex(GLuint &tex, GLuint slot = 0);

	void clearData();
	bool checkEmpty();
	void setInited() { inited = false; }

private:
	void initTrianglesData();
	void initBVHData();

	template<class T>
	void myBindBuffer(GLuint& tbo, GLuint& buffer, std::vector<T>& data, GLuint idx = 0);
};

template<class T>
inline void Scene::myBindBuffer(GLuint& tbo, GLuint& buffer, std::vector<T>& data, GLuint idx)
{
	glGenBuffers(1, &tbo);
	glBindBuffer(GL_TEXTURE_BUFFER, tbo);
	glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(T), &data[0], GL_STATIC_DRAW);

	glGenTextures(1, &buffer);
	glActiveTexture(GL_TEXTURE0 + idx);
	glBindTexture(GL_TEXTURE_BUFFER, buffer);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);
}

class FrameBuffer {
public:
	GLuint FBO; //帧缓冲区
	GLuint RBO;
	std::vector<GLuint> textureColorbuffer;
	std::vector<GLuint> attachment;

	GLuint VAO, VBO, EBO;

	std::vector<float> vertices =
	{
		-1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f
	};

	std::vector<GLuint> indices =
	{
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};


public:
	FrameBuffer();
	~FrameBuffer();

	void Bind();
	void UnBind();

	void Draw(Shader& shader, GLuint slot = 0, GLuint texIndex = 0);
	void BindTexture(GLuint slot = 0, GLuint texIndex = 0);
	void DrawBuffer(GLuint size);
};