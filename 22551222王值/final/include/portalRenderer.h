#ifndef PORTAL_RENDERER_H
#define PORTAL_RENDERER_H

#include "shader.h"
#include "camera.h"
#include "glm/glm.hpp"  // 矩阵运算相关
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

namespace portal {
    
    /// @brief 材质信息
    struct TextureInfo {
        bool isValid;
        unsigned int texture_id;
        int tex_type;
    };
    
    /// @brief 顶点数据，颜色，给材质坐标和法线预留了位置。
    struct Vertex {
        glm::vec3 pos;
        glm::vec4 color;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    class Renderable;

    /// @brief 包含VAO，VBO，材质信息，着色器
    class PortalRenderer {
    public:

        PortalRenderer(int width, int height);
        ~PortalRenderer();

        /// 渲染门框或者门洞, 最重要的接口
        /// @param renderable 可渲染对象
        void RenderPortal(Renderable* renderable);

        /// @brief 传入摄像机矩阵
        /// @param camera 相机，设置view和proj
        void userCameraMatrix(Camera* camera);
        void setViewMat(glm::mat4 view);
        void setProjMat(glm::mat4 proj);

    private:
        glm::mat4 projMat;
        glm::mat4 viewMat;
        glm::ivec2 viewPortSize;    // [width, height]

    };

    // Renderer - 可渲染对象
    class Renderable {
    private:
        unsigned int VAO;
        unsigned int VBO;
        Shader renderShader;
        TextureInfo textureInfo;
        unsigned int type;
        int NumberOfVertices;
        glm::mat4 Model;        // 注意旋转平移之后会变化，可能需要重置

        // 绑定普通纹理
        bool LoadTexture(const char* path);

    public:
        /**
         * @brief Construct a new Renderable object
         * 
         * @param vertices          顶点数据，注意格式为与Vertex结构体相同
         * @param vert_shader_path  几何着色器路径
         * @param frag_shader_path  片段着色器路径
         * @param texture_path      材质路径，空字符串表示没有贴图
         * @param draw_type         绘制类型，默认为三角形
         */
        Renderable( 
            std::vector<Vertex>&& vertices, 
            const std::string vert_shader_path,
            const std::string frag_shader_path,
            const std::string texture_path, 
            unsigned int draw_type = GL_TRIANGLES );
        ~Renderable();

        unsigned int GetVAO() const;
        int GetNumberOfVertices() const;
        glm::mat4 GetModel();
        Shader GetShader() const;
        TextureInfo GetTexture() const;
        unsigned int GetDrawType() const;

        void Translate( glm::vec3 offset );

        void Rotate( float angle, glm::vec3 axis );

        void SetModel( glm::mat4 trans );
    };

}

#endif
