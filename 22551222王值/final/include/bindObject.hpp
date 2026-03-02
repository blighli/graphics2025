#ifndef BIND_OBJECT
#define BIND_OBJECT

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include <iostream>
#include <vector>
#include <cmath>

void bindVAO(std::vector<float> vertices, int attrSize, unsigned int &VAO,
             unsigned int &VBO) {
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    //VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 将顶点数据绑定至当前默认的缓冲中
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 &vertices[0], GL_STATIC_DRAW);
    
    // position attribute
    glVertexAttribPointer(0, attrSize, GL_FLOAT, GL_FALSE, attrSize * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑VAO和VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void bindVAO_EBO(std::vector<float> vertices, int attrSize, std::vector<int> indices,
                 unsigned int &VAO, unsigned int &VBO, unsigned int &EBO) {
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    //VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 将顶点数据绑定至当前默认的缓冲中
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 &vertices[0], GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int),
                 &indices[0], GL_STATIC_DRAW);
    
    // position attribute
    glVertexAttribPointer(0, attrSize, GL_FLOAT, GL_FALSE, attrSize * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑VAO和VBO,EBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

#endif