#include "portalRenderer.h"

using namespace portal;

PortalRenderer::PortalRenderer(int width, int height) {
    this->viewPortSize = glm::ivec2(width, height);
}

void PortalRenderer::RenderPortal(Renderable* renderable) {
    // 绑定VAO
    glBindVertexArray(renderable->GetVAO());

    // 使用材质
    if (renderable->GetTexture().isValid) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(renderable->GetTexture().tex_type,
                      renderable->GetTexture().texture_id);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // 使用着色器
    renderable->GetShader().use();

    // 设置MVP矩阵：Model取自Renderable，View和Proj取自当前的Renderer
    renderable->GetShader().setMat4("model", renderable->GetModel());
    renderable->GetShader().setMat4("view", this->viewMat);
    renderable->GetShader().setMat4("proj", this->projMat);

    // 绘制
    glDrawArrays(renderable->GetDrawType(), 0, renderable->GetNumberOfVertices());
    
}

void PortalRenderer::userCameraMatrix(Camera* camera) {
    this->viewMat = camera->GetViewMatrix();
    this->projMat = glm::mat4(1.0f);
    this->projMat = glm::perspective(glm::radians(camera->Zoom),
                                     (float)viewPortSize.y / (float)viewPortSize.x,
                                     0.1f, 100.0f);
}

void PortalRenderer::setViewMat(glm::mat4 view) {
    this->viewMat = view;
}

void PortalRenderer::setProjMat(glm::mat4 proj) {
    this->projMat = proj;
}

PortalRenderer::~PortalRenderer() {}

//=========================================================

Renderable::Renderable(
    std::vector<Vertex>&& vertices, 
    const std::string vert_shader_path,
    const std::string frag_shader_path, 
    const std::string texture_path, 
    unsigned int draw_type
)
    : renderShader(vert_shader_path.c_str(), frag_shader_path.c_str())  // 着色器
{ 
    this->Model = glm::mat4(1.0f);
    this->type = draw_type;
    this->NumberOfVertices = static_cast<int>(vertices.size());

    // 绑定VAO
    glGenVertexArrays( 1, &VAO );
	glGenBuffers( 1, &VBO ); 

	glBindVertexArray( VAO );
	glBindBuffer( GL_ARRAY_BUFFER, VBO );

    // 申请显存空间来放顶点数据
	glBufferData( GL_ARRAY_BUFFER, NumberOfVertices * sizeof( Vertex ), &vertices.front(), GL_STATIC_DRAW );
	// 绑定顶点位置数据到 0
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)0 );
	glEnableVertexAttribArray( 0 );
	// 绑定顶点颜色数据到 1
	glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)( sizeof( glm::vec3 ) ) );
	glEnableVertexAttribArray( 1 );
	// 绑定顶点UV数据到 2
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)( sizeof( glm::vec3 ) + sizeof( glm::vec4 ) ) );
	glEnableVertexAttribArray( 2 );
	// 绑定顶点法线数据到 3
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)( sizeof( glm::vec3 ) + sizeof( glm::vec4 ) + sizeof( glm::vec2 ) ) );
	glEnableVertexAttribArray( 3 );

    // 纹理，输入空串就不加载
    if (texture_path != "")
        LoadTexture(texture_path.c_str());
    else 
        this->textureInfo.isValid = false;

}

Renderable::~Renderable() {
    glDeleteBuffers( 1, &VBO );
	glDeleteVertexArrays( 1, &VAO );
}

bool Renderable::LoadTexture(const char* path) {
    // 加载并生成纹理
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
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

void Renderable::Translate( glm::vec3 offset ) {
    this->Model = glm::translate(this->Model, offset);
}

void Renderable::Rotate( float angle, glm::vec3 axis ) {
    this->Model = glm::rotate(this->Model, angle, axis);
}

void Renderable::SetModel( glm::mat4 trans ) {
    this->Model = std::move(trans);
}

unsigned int 
Renderable::GetVAO() const {
    return VAO;
}

int Renderable::GetNumberOfVertices() const {
    return NumberOfVertices;
}

glm::mat4 Renderable::GetModel() {
    return Model;
}

Shader Renderable::GetShader() const {
    return renderShader;
}

TextureInfo Renderable::GetTexture() const {
    return textureInfo;
}

unsigned int Renderable::GetDrawType() const {
    return type;
}