#include "Mesh.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "stb_image/stb_image.h"

#include <iostream>

void Mesh::setupMesh()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), &m_Vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int),
		&m_Indices[0], GL_STATIC_DRAW);

	//vertex postion
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	//vertex normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

	//vertex texcoords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

	glBindVertexArray(0);
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures)
	:m_Vertices(vertices), m_Indices(indices), m_Textures(textures)
{
	setupMesh();
}

void Mesh::Draw(Shader shader)
{
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;

	for (unsigned int i = 0; i < m_Textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);

		std::string number;
		std::string name = m_Textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else
			number = std::to_string(specularNr++);

		shader.SetUniform1i(("material." + name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::encodeTriangle(glm::vec3 pos)
{
	if (!triangles.empty() || !textureCoords.empty())
	{
		triangles.clear();
		textureCoords.clear();
	}

	int indicesNum = m_Indices.size();
	for (int i = 2; i < indicesNum; i += 3)
	{
		//std::cout << m_Vertices.size() << std::endl << m_Textures.size() << std::endl;

		Triangle_encoded t;
		Material m;
		t.p1 = m_Vertices[i - 2].Position * 0.1f + pos;
		t.n1 = -1.0f * m_Vertices[i - 2].Normal;

		t.p2 = m_Vertices[i - 1].Position * 0.1f + pos;
		t.n2 = -1.0f * m_Vertices[i - 1].Normal;

		t.p3 = m_Vertices[i].Position * 0.1f + pos;
		t.n3 = -1.0f * m_Vertices[i].Normal;

		t.baseColor = glm::vec3(1.0f);
		t.emissive = glm::vec3(0);
		t.param1 = glm::vec3(0.9f, 0.9f, 0.1f);
		t.param2 = glm::vec3(0.1f);
		t.param3 = glm::vec3(0.1f);
		t.param4 = glm::vec3(0.1f);

		triangles.push_back(t);
		glm::vec2 uv = (m_Vertices[i - 2].TexCoords + m_Vertices[i - 1].TexCoords + m_Vertices[i].TexCoords) / glm::vec2(3);
		textureCoords.push_back(uv);
	}
}

