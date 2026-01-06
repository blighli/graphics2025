#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_sampler.hpp"
#include "misc/texture.hpp"
#include "util/array_view.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct aiScene;
struct aiMaterial;

namespace gameworld
{
    struct ModelVertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 texCoord{0.0f};
        glm::vec3 tangent{1.0f, 0.0f, 0.0f};
        glm::vec3 bitangent{0.0f, 1.0f, 0.0f};
    };

    struct ModelCreateInfo
    {
        VkDescriptorSetLayout materialSetLayout = VK_NULL_HANDLE;
        VkFormat albedoInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat albedoFinalFormat = VK_FORMAT_R8G8B8A8_SRGB;
        VkFormat specularInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat specularFinalFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat normalInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat normalFinalFormat = VK_FORMAT_R8G8B8A8_UNORM;
        bool generateMipmaps = true;
        bool flipUV = true;
        VkIndexType indexType = VK_INDEX_TYPE_UINT32;
        VkSamplerCreateInfo samplerCreateInfo = vulkan::Texture::getDefaultSamplerCreateInfo();
    };

    struct ModelDrawContext
    {
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSet sceneDescriptorSet = VK_NULL_HANDLE;
        uint32_t sceneSetIndex = 0;
        uint32_t materialSetIndex = 1;
        ArrayView<const uint32_t> sceneDynamicOffsets;
        VkShaderStageFlags pushConstantStages = 0;
    };

    class Model
    {
    public:
        Model() = default;
        Model(const std::filesystem::path &modelPath,
              const ModelCreateInfo &createInfo,
              const std::filesystem::path &textureSearchDirectory = {});
        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;
        Model(Model &&) noexcept = default;
        Model &operator=(Model &&) noexcept = default;

        bool loadFromFile(const std::filesystem::path &modelPath,
                          const ModelCreateInfo &createInfo,
                          const std::filesystem::path &textureSearchDirectory = {});

        bool isReady() const { return m_ready; }

        void recordDraw(const vulkan::CommandBuffer &commandBuffer,
                        const ModelDrawContext &context) const;

        void setTranslation(const glm::vec3 &translation);
        void translate(const glm::vec3 &delta);
        void setScale(const glm::vec3 &scale);
        void scale(const glm::vec3 &factor);
        void setRotation(const glm::quat &rotation);
        void setRotation(float radians, const glm::vec3 &axis);
        void rotate(float radians, const glm::vec3 &axis);
        const glm::mat4 &modelMatrix() const;

        VkIndexType getIndexType() const { return m_indexType; }
        const vulkan::VertexBuffer &vertexBuffer() const { return m_vertexBuffer; }
        const vulkan::IndexBuffer &indexBuffer() const { return m_indexBuffer; }

    private:
        struct MeshSubset
        {
            uint32_t firstIndex = 0;
            uint32_t indexCount = 0;
            int32_t vertexOffset = 0;
            uint32_t materialIndex = 0;
        };

        struct MaterialResources
        {
            vulkan::Texture2D albedoTexture;
            vulkan::Texture2D specularTexture;
            vulkan::Texture2D normalTexture;
            glm::vec4 baseColor = glm::vec4(1.0f);
            bool hasAlbedo = false;
            bool hasSpecular = false;
            bool hasNormal = false;
            vulkan::DescriptorSet descriptorSet;
        };

        void updateModelMatrix() const;
        bool initializeSampler(const ModelCreateInfo &createInfo);
        bool initializeDescriptorPool(uint32_t materialCount);
        bool uploadMeshData(const std::vector<ModelVertex> &vertices,
                            const std::vector<uint32_t> &indices32);
        bool buildMaterials(const struct aiScene &scene);
        bool populateMaterial(uint32_t materialIndex, const struct aiMaterial &material);
        bool loadMaterialTexture(vulkan::Texture2D &texture,
                                 std::string_view relativePath,
                                 VkFormat initialFormat,
                                 VkFormat finalFormat);
        std::filesystem::path resolveTexturePath(std::string_view rawPath) const;
        void ensureFallbackTextures();

        bool m_ready = false;
        VkIndexType m_indexType = VK_INDEX_TYPE_UINT32;
        std::filesystem::path m_modelDirectory;
        std::filesystem::path m_textureSearchDirectory;
        ModelCreateInfo m_cachedCreateInfo;

        vulkan::VertexBuffer m_vertexBuffer;
        vulkan::IndexBuffer m_indexBuffer;
        std::vector<MeshSubset> m_meshes;
        std::vector<MaterialResources> m_materials;

        vulkan::DescriptorPool m_materialDescriptorPool;
        vulkan::Sampler m_sampler;
        bool m_samplerValid = false;
        bool m_descriptorPoolValid = false;
        vulkan::Texture2D m_fallbackAlbedoTexture;
        vulkan::Texture2D m_fallbackSpecularTexture;
        vulkan::Texture2D m_fallbackNormalTexture;
        bool m_hasFallbackAlbedo = false;
        bool m_hasFallbackSpecular = false;
        bool m_hasFallbackNormal = false;

        glm::vec3 m_translation{0.0f};
        glm::vec3 m_scale{1.0f};
        glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        mutable glm::mat4 m_modelMatrix{1.0f};
        mutable bool m_transformDirty = true;
    };

    inline void Model::setTranslation(const glm::vec3 &translation)
    {
        m_translation = translation;
        m_transformDirty = true;
    }

    inline void Model::translate(const glm::vec3 &delta)
    {
        m_translation += delta;
        m_transformDirty = true;
    }

    inline void Model::setScale(const glm::vec3 &scale)
    {
        m_scale = scale;
        m_transformDirty = true;
    }

    inline void Model::scale(const glm::vec3 &factor)
    {
        m_scale *= factor;
        m_transformDirty = true;
    }

    inline void Model::setRotation(const glm::quat &rotation)
    {
        m_rotation = glm::normalize(rotation);
        m_transformDirty = true;
    }

    inline void Model::setRotation(float radians, const glm::vec3 &axis)
    {
        m_rotation = glm::angleAxis(radians, glm::normalize(axis));
        m_transformDirty = true;
    }

    inline void Model::rotate(float radians, const glm::vec3 &axis)
    {
        m_rotation = glm::normalize(glm::angleAxis(radians, glm::normalize(axis)) * m_rotation);
        m_transformDirty = true;
    }

    inline const glm::mat4 &Model::modelMatrix() const
    {
        updateModelMatrix();
        return m_modelMatrix;
    }
} // namespace gameworld
