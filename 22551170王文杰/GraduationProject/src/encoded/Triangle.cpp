#include "Triangle.h"

void transmitToBuffer(GLuint& tbo, GLuint& TexBuffer, std::vector<Triangle_encoded> data)
{
	glGenBuffers(1, &tbo);
	glBindBuffer(GL_TEXTURE_BUFFER, tbo);
	glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(Triangle_encoded), &data[0], GL_STATIC_DRAW);

	glGenTextures(1, &TexBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, TexBuffer);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);
}

glm::vec3 GetTriangleCenter(const Triangle_encoded& t)
{	
	glm::vec3 res = (t.p1 + t.p2 + t.p3) / glm::vec3(3, 3, 3);
	return res;
}

bool Triangle_encoded::operator==(Triangle_encoded& t)
{
	if(p1 != t.p1 || p2 != t.p2 || p3 != t.p3)
		return false;
	if (n1 != t.n1 || n2 != t.n2 || n3 != t.n3)
		return false;
	if (emissive != t.emissive || baseColor != t.baseColor || param1 != t.param1)
		return false;
	return true;
}
