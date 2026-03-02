#include "misc/model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <system_error>
#include <new>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gameworld
{
    namespace
    {
        constexpr uint32_t kMaterialTextureSlots = 3;
        constexpr unsigned int kDefaultAssimpFlags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_SortByPType |
            aiProcess_OptimizeMeshes;
    }

    Model::Model(const std::filesystem::path &modelPath,
                 const ModelCreateInfo &createInfo,
                 const std::filesystem::path &textureSearchDirectory)
    {
        loadFromFile(modelPath, createInfo, textureSearchDirectory);
    }

    bool Model::loadFromFile(const std::filesystem::path &modelPath,
                             const ModelCreateInfo &createInfo,
                             const std::filesystem::path &textureSearchDirectory)
    {
        m_ready = false;
        m_meshes.clear();
        m_materials.clear();
        m_hasFallbackAlbedo = false;
        m_hasFallbackSpecular = false;
        m_hasFallbackNormal = false;
        m_descriptorPoolValid = false;

        if (modelPath.empty())
        {
            spdlog::error("Model::loadFromFile: model path is empty.");
            return false;
        }

        if (createInfo.materialSetLayout == VK_NULL_HANDLE)
        {
            spdlog::error("Model::loadFromFile: material descriptor set layout must not be null.");
            return false;
        }

        m_cachedCreateInfo = createInfo;
        m_indexType = createInfo.indexType;
        m_modelDirectory = modelPath.parent_path();
        m_textureSearchDirectory = textureSearchDirectory.empty() ? m_modelDirectory : textureSearchDirectory;

        if (!initializeSampler(createInfo))
        {
            return false;
        }

        Assimp::Importer importer;
        unsigned int processFlags = kDefaultAssimpFlags;
        if (createInfo.flipUV)
        {
            processFlags |= aiProcess_FlipUVs;
        }

        const aiScene *scene = importer.ReadFile(modelPath.string(), processFlags);
        if (!scene || !scene->HasMeshes())
        {
            spdlog::error("Model::loadFromFile: failed to import {}: {}", modelPath.string(), importer.GetErrorString());
            return false;
        }

        uint32_t materialCount = std::max(1u, scene->mNumMaterials);
        if (!initializeDescriptorPool(materialCount))
        {
            return false;
        }

        size_t totalVertices = 0;
        size_t totalIndices = 0;
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh *mesh = scene->mMeshes[meshIndex];
            totalVertices += mesh->mNumVertices;
            totalIndices += static_cast<size_t>(mesh->mNumFaces) * 3u;
        }

        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;
        vertices.reserve(totalVertices);
        indices.reserve(totalIndices);
        m_meshes.reserve(scene->mNumMeshes);

        // process meshes
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh *mesh = scene->mMeshes[meshIndex];
            if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0)
            {
                continue;
            }

            MeshSubset subset;
            subset.vertexOffset = static_cast<int32_t>(vertices.size()); // base vertex index
            subset.firstIndex = static_cast<uint32_t>(indices.size());   // first index in the global index buffer
            subset.materialIndex = std::min<uint32_t>(mesh->mMaterialIndex, materialCount - 1);

            // process vertices
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
            {
                ModelVertex vertex;
                vertex.position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
                if (mesh->HasNormals())
                {
                    vertex.normal = glm::normalize(glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z));
                }
                if (mesh->HasTextureCoords(0))
                {
                    vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
                }
                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.tangent = glm::normalize(glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z));
                    vertex.bitangent = glm::normalize(glm::vec3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z));
                }
                else
                {
                    const glm::vec3 normal = glm::normalize(vertex.normal);
                    glm::vec3 tangent = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal);
                    if (glm::length(tangent) < 1e-4f)
                    {
                        tangent = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normal);
                    }
                    vertex.tangent = glm::normalize(tangent);
                    vertex.bitangent = glm::normalize(glm::cross(normal, vertex.tangent));
                }
                vertices.push_back(vertex);
            }

            // process indices (assume triangulated)
            for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
            {
                const aiFace &face = mesh->mFaces[faceIndex];
                if (face.mNumIndices != 3)
                {
                    continue;
                }
                for (unsigned int i = 0; i < 3; ++i)
                {
                    indices.push_back(static_cast<uint32_t>(face.mIndices[i]));
                }
            }

            subset.indexCount = static_cast<uint32_t>(indices.size()) - subset.firstIndex;
            if (subset.indexCount > 0)
            {
                m_meshes.push_back(subset);
            }
        }

        if (vertices.empty() || indices.empty())
        {
            spdlog::error("Model::loadFromFile: no valid geometry found in {}.", modelPath.string());
            return false;
        }

        // upload mesh data to GPU (Buffer)
        if (!uploadMeshData(vertices, indices))
        {
            spdlog::error("Model::loadFromFile: failed to upload mesh data for {}.", modelPath.string());
            return false;
        }

        // build materials (descriptor sets, textures)
        if (!buildMaterials(*scene))
        {
            spdlog::error("Model::loadFromFile: failed to build materials for {}.", modelPath.string());
            return false;
        }

        m_ready = true;
        return true;
    }

    void Model::recordDraw(const vulkan::CommandBuffer &commandBuffer,
                           const ModelDrawContext &context) const
    {
        if (!m_ready)
        {
            spdlog::warn("Model::recordDraw: model resources are not ready.");
            return;
        }
        if (context.pipelineLayout == VK_NULL_HANDLE)
        {
            spdlog::error("Model::recordDraw: pipeline layout is null.");
            return;
        }
        if (m_meshes.empty())
        {
            return;
        }

        VkBuffer vertexBuffers[] = {m_vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, m_indexType);

        if (context.pushConstantStages != 0)
        {
            const glm::mat4 modelMatrixValue = modelMatrix();
            vkCmdPushConstants(
                commandBuffer,
                context.pipelineLayout,
                context.pushConstantStages,
                0,
                sizeof(glm::mat4),
                glm::value_ptr(modelMatrixValue));
        }

        if (context.sceneDescriptorSet != VK_NULL_HANDLE)
        {
            const uint32_t dynamicCount = static_cast<uint32_t>(context.sceneDynamicOffsets.count());
            const uint32_t *dynamicData = dynamicCount > 0 ? context.sceneDynamicOffsets.pointer() : nullptr;
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                context.pipelineLayout,
                context.sceneSetIndex,
                1,
                &context.sceneDescriptorSet,
                dynamicCount,
                dynamicData);
        }

        for (const MeshSubset &subset : m_meshes)
        {
            VkDescriptorSet materialSet = VK_NULL_HANDLE;
            if (subset.materialIndex < m_materials.size())
            {
                materialSet = m_materials[subset.materialIndex].descriptorSet;
            }

            if (materialSet != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    context.pipelineLayout,
                    context.materialSetIndex,
                    1,
                    &materialSet,
                    0,
                    nullptr);
            }

            vkCmdDrawIndexed(
                commandBuffer,
                subset.indexCount,
                1,
                subset.firstIndex,
                subset.vertexOffset,
                0);
        }
    }

    bool Model::initializeSampler(const ModelCreateInfo &createInfo)
    {
        VkSamplerCreateInfo samplerInfo = createInfo.samplerCreateInfo;
        const bool usingDefaultSampler = (samplerInfo.sType == 0);
        if (usingDefaultSampler)
        {
            samplerInfo = vulkan::Texture::getDefaultSamplerCreateInfo();
        }

        m_sampler.~Sampler();
        new (&m_sampler) vulkan::Sampler();
        if (m_sampler.create(samplerInfo) != VK_SUCCESS)
        {
            spdlog::error("Model::initializeSampler: failed to create sampler.");
            return false;
        }

        m_samplerValid = true;
        return true;
    }

    bool Model::initializeDescriptorPool(uint32_t materialCount)
    {
        if (materialCount == 0)
        {
            materialCount = 1;
        }

        m_materialDescriptorPool.~DescriptorPool();
        new (&m_materialDescriptorPool) vulkan::DescriptorPool();
        VkDescriptorPoolSize poolSize = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = materialCount * kMaterialTextureSlots};
        ArrayView<const VkDescriptorPoolSize> poolSizes(&poolSize, 1);
        if (m_materialDescriptorPool.create(materialCount, poolSizes) != VK_SUCCESS)
        {
            spdlog::error("Model::initializeDescriptorPool: failed to create descriptor pool.");
            return false;
        }

        m_descriptorPoolValid = true;
        return true;
    }

    bool Model::uploadMeshData(const std::vector<ModelVertex> &vertices,
                               const std::vector<uint32_t> &indices32)
    {
        if (vertices.empty() || indices32.empty())
        {
            return false;
        }

        const VkDeviceSize vertexBufferSize = sizeof(ModelVertex) * vertices.size();
        m_vertexBuffer.~VertexBuffer();
        new (&m_vertexBuffer) vulkan::VertexBuffer(vertexBufferSize);
        m_vertexBuffer.transferData(vertices.data(), vertexBufferSize);

        m_indexBuffer.~IndexBuffer();
        if (m_indexType == VK_INDEX_TYPE_UINT16)
        {
            // transform all indices to int16 
            std::vector<uint16_t> indices16(indices32.size());
            for (size_t i = 0; i < indices32.size(); ++i)
            {
                if (indices32[i] > std::numeric_limits<uint16_t>::max())
                {
                    spdlog::error("Model::uploadMeshData: index {} exceeds uint16 range. Use VK_INDEX_TYPE_UINT32 instead.", indices32[i]);
                    return false;
                }
                indices16[i] = static_cast<uint16_t>(indices32[i]);
            }

            VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices16.size();
            new (&m_indexBuffer) vulkan::IndexBuffer(indexBufferSize);
            m_indexBuffer.transferData(indices16.data(), indexBufferSize);
        }
        else
        {
            VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices32.size();
            new (&m_indexBuffer) vulkan::IndexBuffer(indexBufferSize);
            m_indexBuffer.transferData(indices32.data(), indexBufferSize);
        }

        return true;
    }

    bool Model::buildMaterials(const aiScene &scene)
    {
        if (!m_descriptorPoolValid)
        {
            spdlog::error("Model::buildMaterials: descriptor pool is not initialized.");
            return false;
        }

        uint32_t materialCount = std::max(1u, scene.mNumMaterials);
        m_materials.clear();
        m_materials.resize(materialCount);

        std::vector<vulkan::DescriptorSet> allocatedSets(materialCount);
        std::vector<VkDescriptorSetLayout> layouts(materialCount, m_cachedCreateInfo.materialSetLayout);
        if (m_materialDescriptorPool.allocateSets(allocatedSets, layouts) != VK_SUCCESS)
        {
            spdlog::error("Model::buildMaterials: failed to allocate descriptor sets.");
            return false;
        }

        for (uint32_t i = 0; i < materialCount; ++i)
        {
            m_materials[i].descriptorSet = std::move(allocatedSets[i]);
            if (i < scene.mNumMaterials)
            {
                populateMaterial(i, *scene.mMaterials[i]);
            }
            ensureFallbackTextures();

            // write descriptor set
            const vulkan::Texture2D &albedoTexture = m_materials[i].hasAlbedo ? m_materials[i].albedoTexture : m_fallbackAlbedoTexture;
            const vulkan::Texture2D &specularTexture = m_materials[i].hasSpecular ? m_materials[i].specularTexture : m_fallbackSpecularTexture;
            const vulkan::Texture2D &normalTexture = m_materials[i].hasNormal ? m_materials[i].normalTexture : m_fallbackNormalTexture;

            VkDescriptorImageInfo albedoInfo = albedoTexture.getDescriptorImageInfo(m_sampler);
            VkDescriptorImageInfo specularInfo = specularTexture.getDescriptorImageInfo(m_sampler);
            VkDescriptorImageInfo normalInfo = normalTexture.getDescriptorImageInfo(m_sampler);
            m_materials[i].descriptorSet.write(ArrayView(albedoInfo), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
            m_materials[i].descriptorSet.write(ArrayView(specularInfo), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
            m_materials[i].descriptorSet.write(ArrayView(normalInfo), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);
        }

        return true;
    }

    bool Model::populateMaterial(uint32_t materialIndex, const aiMaterial &material)
    {
        MaterialResources &resources = m_materials[materialIndex];

        aiColor4D diffuseColor;
        if (material.Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
        {
            resources.baseColor = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
        }

        auto tryLoadTexture = [&](aiTextureType type,
                                  vulkan::Texture2D &texture,
                                  VkFormat initialFormat,
                                  VkFormat finalFormat) -> bool
        {
            if (material.GetTextureCount(type) == 0)
            {
                return false;
            }

            aiString texturePath;
            if (material.GetTexture(type, 0, &texturePath) != AI_SUCCESS)
            {
                return false;
            }

            if (!texturePath.length || texturePath.C_Str()[0] == '*')
            {
                spdlog::warn("Model::populateMaterial: embedded textures are not supported ({}).", texturePath.C_Str());
                return false;
            }

            return loadMaterialTexture(texture, texturePath.C_Str(), initialFormat, finalFormat);
        };

        resources.hasAlbedo = tryLoadTexture(
            aiTextureType_DIFFUSE,
            resources.albedoTexture,
            m_cachedCreateInfo.albedoInitialFormat,
            m_cachedCreateInfo.albedoFinalFormat);

        resources.hasSpecular = tryLoadTexture(
            aiTextureType_SPECULAR,
            resources.specularTexture,
            m_cachedCreateInfo.specularInitialFormat,
            m_cachedCreateInfo.specularFinalFormat);

        resources.hasNormal = tryLoadTexture(
            aiTextureType_NORMALS,
            resources.normalTexture,
            m_cachedCreateInfo.normalInitialFormat,
            m_cachedCreateInfo.normalFinalFormat);

        if (!resources.hasNormal)
        {
            resources.hasNormal = tryLoadTexture(
                aiTextureType_HEIGHT,
                resources.normalTexture,
                m_cachedCreateInfo.normalInitialFormat,
                m_cachedCreateInfo.normalFinalFormat);
        }

        return resources.hasAlbedo || resources.hasSpecular || resources.hasNormal;
    }

    bool Model::loadMaterialTexture(vulkan::Texture2D &texture,
                                    std::string_view relativePath,
                                    VkFormat initialFormat,
                                    VkFormat finalFormat)
    {
        if (relativePath.empty())
        {
            return false;
        }

        std::filesystem::path resolved = resolveTexturePath(relativePath);
        if (resolved.empty())
        {
            spdlog::warn("Model::loadMaterialTexture: failed to resolve texture path {}.", relativePath);
            return false;
        }

        texture.create(
            resolved.string().c_str(),
            initialFormat,
            finalFormat,
            m_cachedCreateInfo.generateMipmaps);
        return true;
    }

    std::filesystem::path Model::resolveTexturePath(std::string_view rawPath) const
    {
        if (rawPath.empty())
        {
            return {};
        }

        std::filesystem::path candidate(rawPath);
        candidate = candidate.lexically_normal();

        auto exists = [](const std::filesystem::path &path) -> bool
        {
            std::error_code ec;
            return std::filesystem::exists(path, ec);
        };

        if (candidate.is_absolute() && exists(candidate))
        {
            return candidate;
        }

        if (!m_textureSearchDirectory.empty())
        {
            std::filesystem::path combined = m_textureSearchDirectory / candidate;
            if (exists(combined))
            {
                return combined;
            }
        }

        if (!m_modelDirectory.empty())
        {
            std::filesystem::path combined = m_modelDirectory / candidate;
            if (exists(combined))
            {
                return combined;
            }
        }

        return {};
    }

    void Model::ensureFallbackTextures()
    {
        const std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
        const std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
        VkExtent2D extent{1, 1};

        auto sanitizeFormat = [](VkFormat format, VkFormat fallback)
        {
            if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                return format;
            }
            return fallback;
        };

        if (!m_hasFallbackAlbedo)
        {
            VkFormat albedoFormat = sanitizeFormat(m_cachedCreateInfo.albedoFinalFormat, VK_FORMAT_R8G8B8A8_UNORM);
            m_fallbackAlbedoTexture.create(whitePixel.data(), extent, albedoFormat, albedoFormat, false);
            m_hasFallbackAlbedo = true;
        }

        if (!m_hasFallbackSpecular)
        {
            VkFormat specularFormat = sanitizeFormat(m_cachedCreateInfo.specularFinalFormat, VK_FORMAT_R8G8B8A8_UNORM);
            m_fallbackSpecularTexture.create(whitePixel.data(), extent, specularFormat, specularFormat, false);
            m_hasFallbackSpecular = true;
        }

        if (!m_hasFallbackNormal)
        {
            VkFormat normalFormat = sanitizeFormat(m_cachedCreateInfo.normalFinalFormat, VK_FORMAT_R8G8B8A8_UNORM);
            m_fallbackNormalTexture.create(normalPixel.data(), extent, normalFormat, normalFormat, false);
            m_hasFallbackNormal = true;
        }
    }

    void Model::updateModelMatrix() const
    {
        if (!m_transformDirty)
        {
            return;
        }

        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_translation);
        const glm::mat4 rotation = glm::mat4_cast(m_rotation);
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);

        m_modelMatrix = translation * rotation * scale;
        m_transformDirty = false;
    }

} // namespace gameworld
