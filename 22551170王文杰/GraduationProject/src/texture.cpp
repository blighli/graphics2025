#include "texture.h"

Texture::Texture(const void* data, int size)
{
	glGenBuffers(1, &tb);
	glBindBuffer(GL_TEXTURE_BUFFER, tb);
	glBufferData(GL_TEXTURE_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_BUFFER, tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tb);
}

Texture::~Texture()
{
	glDeleteBuffers(1, &tb);
	glDeleteTextures(1, &tex);
}

void Texture::Bind() const
{
	glBindBuffer(GL_TEXTURE_BUFFER, tb);
	glBindTexture(GL_TEXTURE_BUFFER, tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tb);
}

void Texture::UnBind() const
{
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}
