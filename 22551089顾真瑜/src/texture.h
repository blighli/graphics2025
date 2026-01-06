#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "stb_image.h"

#include <vector>
#include <string>
#include <iostream>
using std::vector;
using std::string;

struct Texture{
	unsigned int id;
	string name;
	string path;	// for model load improvement
};

/// @brief load a new texture
/// @param texture_name texture name in shader
/// @param texture_file texture file path
/// @param flip flip texture or not
inline Texture* loadTexture(string texture_name, const string texture_file, bool flip = true, int wrap_stype = GL_REPEAT){

	Texture* texture = new Texture();
	unsigned int tid;
	glGenTextures(1, &tid);
	texture->id = tid;
	texture->name = texture_name;
	texture->path = texture_file;

	glBindTexture(GL_TEXTURE_2D, tid);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_stype);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_stype);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(flip)	stbi_set_flip_vertically_on_load(true);

	int width, height, nrChannels;
	unsigned char *data = stbi_load(texture_file.c_str(), &width, &height, &nrChannels, 0);


	// load texture img to GL_TEXTURE_2D
	if(data){
		GLenum format;
		if(nrChannels == 1) {
			// is specular, transform to RGB
			unsigned char* rgbData = new unsigned char[width * height * 3];
			for(int i = 0; i < width * height; ++i) {
				unsigned char value = data[i];
				rgbData[i*3] = value;      // R
				rgbData[i*3+1] = value;    // G
				rgbData[i*3+2] = value;    // B
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbData);
			glGenerateMipmap(GL_TEXTURE_2D);

			delete[] rgbData;
			std::cout << "Converted 1-channel to RGB format" << std::endl;
			stbi_image_free(data);
			return texture;

		} else if(nrChannels == 3) {
			format = GL_RGB;
		} else if(nrChannels == 4) {
			format = GL_RGBA;
		} else {
			std::cout << "Unsupported number of channels: " << nrChannels << std::endl;
        	stbi_image_free(data);
			return texture;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

	} else {
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);
	return texture;
}

/// @brief Note that the face order must be: right, left, up, bottom, back and front
/// @return cube texture's textureID
inline unsigned int loadCubemap(vector<string> face_paths){
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	int width, height, nrChannels;
	unsigned char* data;
	for(unsigned int i = 0; i < face_paths.size(); i++){
		data = stbi_load(face_paths[i].c_str(), &width, &height, &nrChannels, 0);
		if(data){
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else{
			std::cout << "Cubemap texture failed to load at path: " << face_paths[i] << std::endl;
            stbi_image_free(data);
		}
	}

	return textureID;
}





#endif
