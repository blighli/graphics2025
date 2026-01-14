#include "scene_func.h"

Scene_Triangle::Scene_Triangle()
{
    triangle_shader = new Shader("./shader/triangleShader.vs", "./shader/triangleShader.fs");

    // triangle
	float vertices[] = {
        0.0f,  0.577f, 0.0f,  // 上顶点
        -0.5f, -0.289f, 0.0f,  // 左下顶点
        0.5f, -0.289f, 0.0f   // 右下顶点
	};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Scene_Triangle::~Scene_Triangle()
{
    delete triangle_shader;
}

void Scene_Triangle::renderFunc(){
    // refresh screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    triangle_shader->use();
    glm::mat4 transform = glm::mat4(1.0);
	transform = glm::rotate(transform, glm::radians(rotation), glm::vec3(0.0, 0.0, 1.0));
    triangle_shader->setMat4("transform", transform);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    rotation += 0.1;

}
