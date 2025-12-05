#include "Scene.h"

#include <random>
#include <iostream>

#include "Camera.h"
//--------random function------------------
//extern std::uniform_real_distribution<> dis(0.0, 1.0);
//extern std::random_device rd;
//extern std::mt19937 gen(rd());
extern double randf();
extern glm::vec3 randomVec3();
extern glm::vec3 randomDir(glm::vec3 n);

extern newCamera newcamera;
//-----------end------------------------

Scene::Scene()
	:inited(false), modified(false), tbo(0), texBuffer(0), bvh_tbo(0), bvh_texBuffer(0)
{
	vertices = {
		1.0f,  1.0f, 0.0f,  // top right
		1.0f, -1.0f, 0.0f,  // bottom right
	   -1.0f, -1.0f, 0.0f,  // bottom left
	   -1.0f,  1.0f, 0.0f   // top left 
	};

	indices = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

Scene::Scene(std::vector<shape*> model)
	:inited(false), modified(false), tbo(0), texBuffer(0), bvh_tbo(0), bvh_texBuffer(0)
{
	vertices = {
		1.0f,  1.0f, 0.0f,  // top right
		1.0f, -1.0f, 0.0f,  // bottom right
	   -1.0f, -1.0f, 0.0f,  // bottom left
	   -1.0f,  1.0f, 0.0f   // top left 
	};

	indices = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

}

Scene::~Scene()
{
	glDeleteBuffers(1, &tbo);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &bvh_tbo);
	glDeleteVertexArrays(1, &VAO);
	glDeleteTextures(1, &texBuffer);
	glDeleteTextures(1, &bvh_texBuffer);
}

void Scene::push(shape* s)
{
	models.push_back(s);
}

void Scene::push(Light light)
{
	Lights.push_back(light);
}


void Scene::Render(Shader& shader)
{
	if (!inited) {
		initTrianglesData();
		initBVHData();

		// 绑定缓冲区
		myBindBuffer(tbo, texBuffer, trianglesData, 0);
		myBindBuffer(bvh_tbo, bvh_texBuffer, BVHData, 1);

		inited = true;
	}

	shader.Bind();
	
	shader.SetUniform1i("triangles", 0);
	shader.SetUniform1i("bvh", 1);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

void Scene::Draw(Shader& shader)
{	
	if (!inited) {
		initTrianglesData();
		initBVHData();

		// 绑定缓冲区
		myBindBuffer(tbo, texBuffer, trianglesData, 0);
		myBindBuffer(bvh_tbo, bvh_texBuffer, BVHData, 1);

		inited = true;
	}

	shader.Bind();
	shader.SetUniform1i("triangles", 0);
	shader.SetUniform1i("bvh", 1);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

void Scene::BindTex(GLuint& tex, GLuint slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, tex);
}


void Scene::initTrianglesData()
{
	int n = models.size();
	for (int i = 0; i < n; i++) {
		auto& it = models[i];
		auto data = it->getCodedData();

		trianglesData.insert(trianglesData.end(), data.begin(), data.end());	//把每一个数据存入数据库
	}
	std::cout << "三角形共" << trianglesData.size() << "个" << std::endl;


}

void Scene::initBVHData()
{
	int n = trianglesData.size();
	BVHNode tNode;
	tNode.left = 255;
	tNode.right = 128;
	tNode.n = 30;
	tNode.AA = { 1,1,0 };
	tNode.BB = { 0,1,0 };

	std::vector<BVHNode> nodes{ tNode };

	// 减少BVH层数会变完整 why？0
	BuildBVH(trianglesData, nodes, 0, n - 1, 8);
	
	std::cout << "BVH共" << nodes.size() << "个节点" << std::endl;

	int nNode = nodes.size();
	if (!BVHData.empty())
		BVHData.clear();
	for (int i = 0; i < nNode; i++)
	{
		//std::cout << i << std::endl;
		BVHData.push_back(encodeBVH(nodes[i]));
	}

}

FrameBuffer::FrameBuffer()
	:FBO(0), VAO(0), VBO(0), EBO(0)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	// framebuffer
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// create a color attachment texture
	textureColorbuffer = std::vector<GLuint>(6);
	for (int i = 0; i < 6; i++) {
		glGenTextures(1, &textureColorbuffer[i]);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textureColorbuffer[i], 0);
		attachment.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	glDrawBuffers(1, &attachment[0]);
	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	//unsigned int rbo;
	//glGenRenderbuffers(1, &rbo);
	//glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it

	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now    
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
	glDeleteFramebuffers(1, &FBO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void FrameBuffer::Bind() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void FrameBuffer::UnBind() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void FrameBuffer::BindTexture(GLuint slot, GLuint texIndex)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer[texIndex]);
}

void FrameBuffer::DrawBuffer(GLuint size)
{
	//size = glm::min(size, (GLuint)attachment.size());

	glDrawBuffers(size, &attachment[0]);
}

void FrameBuffer::Draw(Shader& shader, GLuint slot, GLuint texIndex)
{
	shader.Bind();

	glBindVertexArray(VAO);
	
	BindTexture(slot, texIndex);

	glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	shader.UnBind();
}

void Scene::clearData()
{
	models.clear();
	trianglesData.clear();
	BVHData.clear();
}

bool Scene::checkEmpty()
{
	return trianglesData.empty() && BVHData.empty();
}
