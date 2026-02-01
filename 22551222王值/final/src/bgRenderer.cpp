#include "bgRenderer.h"
#include "shader.h"

using namespace bg;

BgRenderer::BgRenderer(int width, int height) { this->viewPortSize = glm::ivec2(width, height); }

void BgRenderer::RenderBg(Renderable* renderable) {
    // 绑定VAO
    glBindVertexArray(renderable->GetVAO());

    // 使用材质
    if (renderable->GetTexture().isValid) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(renderable->GetTexture().tex_type, renderable->GetTexture().texture_id);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // 使用着色器
    renderable->GetShader()->use();

    // 设置MVP矩阵：Model取自Renderable，View和Proj取自当前的Renderer
    renderable->GetShader()->setMat4("model", renderable->GetModel());
    renderable->GetShader()->setMat4("view", this->viewMat);
    renderable->GetShader()->setMat4("proj", this->projMat);

    // 绘制
    glDrawArrays(renderable->GetDrawType(), 0, renderable->GetNumberOfVertices());
}

void BgRenderer::userCameraMatrix(Camera* camera) {
    this->viewMat = camera->GetViewMatrix();
    this->projMat = glm::mat4(1.0f);
    this->projMat =
        glm::perspective(glm::radians(camera->Zoom), (float)viewPortSize.y / (float)viewPortSize.x, 0.1f, 100.0f);
}

void BgRenderer::setViewMat(glm::mat4 view) { this->viewMat = view; }

void BgRenderer::setProjMat(glm::mat4 proj) { this->projMat = proj; }

BgRenderer::~BgRenderer() {}

//=========================================================

Renderable::Renderable(const std::string obj_path, [[maybe_unused]]const std::string vert_shader_path,
                       [[maybe_unused]]const std::string frag_shader_path, const std::string texture_path, unsigned int draw_type)
    : objModel(obj_path) {
    this->Model = glm::mat4(1.0f);
    this->type = draw_type;

    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    std::vector<int> indexData;
    std::vector<float> normalData;

    // 重复使用顶点
    for (int i = 0; i < static_cast<int>(objModel.faces.size()); i++) {
        vertexData.push_back(objModel.vertices[objModel.faces[i].v1].x);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v1].y);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v1].z);

        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt1].u);
        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt1].v);

        normalData.push_back(objModel.normals[objModel.faces[i].vn1].nx);
        normalData.push_back(objModel.normals[objModel.faces[i].vn1].ny);
        normalData.push_back(objModel.normals[objModel.faces[i].vn1].nz);

        vertexData.push_back(objModel.vertices[objModel.faces[i].v2].x);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v2].y);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v2].z);

        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt2].u);
        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt2].v);

        normalData.push_back(objModel.normals[objModel.faces[i].vn2].nx);
        normalData.push_back(objModel.normals[objModel.faces[i].vn2].ny);
        normalData.push_back(objModel.normals[objModel.faces[i].vn2].nz);

        vertexData.push_back(objModel.vertices[objModel.faces[i].v3].x);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v3].y);
        vertexData.push_back(objModel.vertices[objModel.faces[i].v3].z);

        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt3].u);
        texCoordData.push_back(objModel.texcoords[objModel.faces[i].vt3].v);

        normalData.push_back(objModel.normals[objModel.faces[i].vn3].nx);
        normalData.push_back(objModel.normals[objModel.faces[i].vn3].ny);
        normalData.push_back(objModel.normals[objModel.faces[i].vn3].nz);

        indexData.push_back(i * 3);
        indexData.push_back(i * 3 + 1);
        indexData.push_back(i * 3 + 2);
    }
    this->NumberOfVertices = static_cast<int>(vertexData.size() / 3);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &TexCoordVBO);
    glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO);
    glBufferData(GL_ARRAY_BUFFER, texCoordData.size() * sizeof(float), texCoordData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &NormalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, NormalVBO);
    glBufferData(GL_ARRAY_BUFFER, normalData.size() * sizeof(float), normalData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof(int), indexData.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    // 纹理，输入空串就不加载
    if (texture_path != "") LoadTexture(texture_path.c_str());
    else this->textureInfo.isValid = false;
}

Renderable::~Renderable() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

bool Renderable::LoadTexture(const char* path) {
    // 加载并生成纹理
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    // auto gl_color_channel = (nrChannels == 3 ? GL_RGB : GL_RGBA);
    auto gl_color_channel = (nrChannels == 3 ? GL_RGB : (nrChannels == 4 ? GL_RGBA : GL_RGB)); // 颜色格式
    if (data) {
        unsigned int texture;
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        // 为当前绑定的纹理对象设置环绕、过滤方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, gl_color_channel, width, height, 0, gl_color_channel, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        this->textureInfo.isValid = true;
        this->textureInfo.texture_id = texture;
        this->textureInfo.tex_type = GL_TEXTURE_2D;
        std::cout << "Texture loaded: " << path << std::endl;
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
        return false;
    }
    stbi_image_free(data);
    return true;
}

void Renderable::Translate(glm::vec3 offset) { this->Model = glm::translate(this->Model, offset); }

void Renderable::Rotate(float angle, glm::vec3 axis) { this->Model = glm::rotate(this->Model, angle, axis); }

void Renderable::SetModel(glm::mat4 trans) { this->Model = std::move(trans); }

unsigned int Renderable::GetVAO() const { return VAO; }

int Renderable::GetNumberOfVertices() const { return NumberOfVertices; }

glm::mat4 Renderable::GetModel() { return Model; }

void Renderable::SetShader(Shader* shader) { this->renderShader = shader; }

Shader* Renderable::GetShader() const { return renderShader; }

TextureInfo Renderable::GetTexture() const { return textureInfo; }

unsigned int Renderable::GetDrawType() const { return type; }

void Renderable::GetP1P2(glm::vec3 &p1, glm::vec3 &p2) {
    p1 = objModel.maxP;
    p2 = objModel.minP;
    glm::vec4 temp1 = Model * glm::vec4(p1, 1.f);
    glm::vec4 temp2 = Model * glm::vec4(p2, 1.f);
    p1 = std::move(glm::vec3(temp1.x, temp1.y, temp1.z));
    p2 = std::move(glm::vec3(temp2.x, temp2.y, temp2.z));
}