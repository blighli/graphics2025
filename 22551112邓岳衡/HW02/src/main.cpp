#include "learn_vulkan.hpp"
#include "easy_vulkan.hpp"
#include "glfw_wrapper.hpp"
#include "vulkan_manager.hpp"
#include "vulkan_fence.hpp"
#include "vulkan_semaphore.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_shader_module.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_sampler.hpp"
#include "vulkan_frame_buffer.hpp"
#include "vulkan_image.hpp"

#include "misc/camera.hpp"
#include "misc/sphere.hpp"

#include "util/util.hpp"
#include "util/array_view.hpp"
#include "util/macro.hpp"
#include "util/vkresult_wrapper.hpp"

#include <thread>
#include <chrono>
#include <array>
#include <vector>
#include <cstring>
#include <optional>
#include <algorithm>

using namespace vulkan;
using namespace gameworld;

struct SceneUniform
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct PlanetUniform
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 color;
};

constexpr uint32_t kPlanetCount = 3;
constexpr VkFormat kPickingFormat = VK_FORMAT_R8G8B8A8_UNORM;

const std::array<glm::vec4, kPlanetCount> kPickingColors = {
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)};
const std::array<glm::u8vec4, kPlanetCount> kPickingColorBytes = {
    glm::u8vec4(255, 0, 0, 255),
    glm::u8vec4(0, 255, 0, 255),
    glm::u8vec4(0, 0, 255, 255)};
const glm::vec4 kOutlineColorAndScale = glm::vec4(1.0f, 0.85f, 0.2f, 0.05f); // w component stores outline scale

DescriptorSetLayout descriptorSetLayout_uniform;
DescriptorSetLayout descriptorSetLayout_material;
PipelineLayout pipelineLayout_planet;
Pipeline pipeline_planet;
PipelineLayout pipelineLayout_picking;
Pipeline pipeline_picking;
PipelineLayout pipelineLayout_outline;
Pipeline pipeline_outline;

RenderPass pickingRenderPass;
FrameBuffer pickingFrameBuffer;
ImageMemory pickingImageMemory;
ImageView pickingImageView;
BufferMemory pickingReadbackBuffer;

std::optional<uint32_t> selectedPlanet;
bool mouseLeftPressedLastFrame = false;
bool pickRequestPending = false;
bool pickReadbackPending = false;
glm::ivec2 pendingPickPixel{0, 0};
glm::ivec2 activePickPixel{0, 0};

void recreatePickingTargets();
void destroyPickingTargets();
void createPickingResources();
void createPickingReadbackBuffer();
void createPickingPipeline();
void createOutlinePipeline();
void resolvePickingResult();

Camera camera;

const auto &RenderPassAndFrameBuffersWithDepth()
{
    static const auto &rpwf_depth = easy_vulkan::createRenderPassFBWithDepth();
    return rpwf_depth;
}

void createPickingResources()
{
    static bool renderPassInitialized = false;
    if (!renderPassInitialized)
    {
        VkAttachmentDescription colorAttachment = {
            .format = kPickingFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};   // for Buffer copy after renderpass

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef};

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT};

        VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency};

        pickingRenderPass.create(renderPassInfo);
        VulkanManager::getManager().registerDestroySwapchainCallback(destroyPickingTargets);
        VulkanManager::getManager().registerCreateSwapchainCallback(recreatePickingTargets);
        renderPassInitialized = true;
    }

    recreatePickingTargets();
}

void recreatePickingTargets()
{
    destroyPickingTargets();

    const VkExtent2D extent = VulkanManager::getManager().getSwapchainCreateInfo().imageExtent;
    if (extent.width == 0 || extent.height == 0)
    {
        return;
    }

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = kPickingFormat,
        .extent = {extent.width, extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    pickingImageMemory.create(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = pickingImageMemory.getImage(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = kPickingFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    pickingImageView.create(viewInfo);

    VkImageView attachment = pickingImageView;
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pickingRenderPass,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .width = extent.width,
        .height = extent.height,
        .layers = 1};
    pickingFrameBuffer.create(framebufferInfo);
}

void destroyPickingTargets()
{
    pickingFrameBuffer.~FrameBuffer();
    pickingImageView.~ImageView();
    pickingImageMemory.~ImageMemory();
}

void createPickingReadbackBuffer()
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    pickingReadbackBuffer.~BufferMemory();
    pickingReadbackBuffer.create(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void createPickingPipeline()
{
    static ShaderModule vert("shaders/spv/planet.vert.spv");
    static ShaderModule frag("shaders/spv/picking.frag.spv");
    static VkPipelineShaderStageCreateInfo shaderStages[] = {
        vert.getStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, "main"),
        frag.getStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

    auto createPipelineInternal = []
    {
        GraphicsPipelineCreateInfoPack infoPack;
        infoPack.createInfo.layout = pipelineLayout_picking;
        infoPack.createInfo.renderPass = pickingRenderPass;
        infoPack.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        infoPack.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        infoPack.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        infoPack.viewports.emplace_back(VkViewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(windowSize.width),
            .height = static_cast<float>(windowSize.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f});
        infoPack.scissors.emplace_back(VkRect2D{
            .offset = {0, 0},
            .extent = windowSize});

        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(SphereVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        infoPack.bindingDescriptions.push_back(bindingDescription);

        VkVertexInputAttributeDescription attributes[] = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, pos)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, normal)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SphereVertex, texCoord)}};
        infoPack.attributeDescriptions.assign(std::begin(attributes), std::end(attributes));

        infoPack.colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

        infoPack.shaderStages.assign(std::begin(shaderStages), std::end(shaderStages));
        infoPack.updateAllVectorData();

        pipeline_picking.create(infoPack);
    };

    auto destroyPipelineInternal = []
    {
        pipeline_picking.~Pipeline();
    };

    VulkanManager::getManager().registerCreateSwapchainCallback(createPipelineInternal);
    VulkanManager::getManager().registerDestroySwapchainCallback(destroyPipelineInternal);
    createPipelineInternal();
}

void createOutlinePipeline()
{
    static ShaderModule vert("shaders/spv/outline.vert.spv");
    static ShaderModule frag("shaders/spv/outline.frag.spv");
    static VkPipelineShaderStageCreateInfo shaderStages[] = {
        vert.getStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, "main"),
        frag.getStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

    auto createPipelineInternal = []
    {
        GraphicsPipelineCreateInfoPack infoPack;
        infoPack.createInfo.layout = pipelineLayout_outline;
        infoPack.createInfo.renderPass = RenderPassAndFrameBuffersWithDepth().renderPass;
        infoPack.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        infoPack.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        infoPack.viewports.emplace_back(VkViewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(windowSize.width),
            .height = static_cast<float>(windowSize.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f});
        infoPack.scissors.emplace_back(VkRect2D{
            .offset = {0, 0},
            .extent = windowSize});

        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(SphereVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        infoPack.bindingDescriptions.push_back(bindingDescription);

        VkVertexInputAttributeDescription attributes[] = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, pos)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, normal)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SphereVertex, texCoord)}};
        infoPack.attributeDescriptions.assign(std::begin(attributes), std::end(attributes));

        infoPack.depthStencilState.depthTestEnable = VK_TRUE;
        infoPack.depthStencilState.depthWriteEnable = VK_FALSE;
        infoPack.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        infoPack.rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;

        infoPack.colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

        infoPack.shaderStages.assign(std::begin(shaderStages), std::end(shaderStages));
        infoPack.updateAllVectorData();

        pipeline_outline.create(infoPack);
    };

    auto destroyPipelineInternal = []
    {
        pipeline_outline.~Pipeline();
    };

    VulkanManager::getManager().registerCreateSwapchainCallback(createPipelineInternal);
    VulkanManager::getManager().registerDestroySwapchainCallback(destroyPipelineInternal);
    createPipelineInternal();
}

void resolvePickingResult()
{
    if (!pickReadbackPending)
    {
        return;
    }

    std::array<uint8_t, 4> pixel = {0, 0, 0, 0};
    pickingReadbackBuffer.retrieveData(pixel.data(), static_cast<VkDeviceSize>(pixel.size()));

    pickReadbackPending = false;
    selectedPlanet.reset();
    for (uint32_t i = 0; i < kPlanetCount; ++i)
    {
        if (pixel[0] == kPickingColorBytes[i].r &&
            pixel[1] == kPickingColorBytes[i].g &&
            pixel[2] == kPickingColorBytes[i].b)
        {
            selectedPlanet = i;
            return;
        }
    }
}

void createPipeline()
{
    static ShaderModule vert("shaders/spv/planet.vert.spv");
    static ShaderModule frag("shaders/spv/planet.frag.spv");
    static VkPipelineShaderStageCreateInfo shaderStages[] = {
        vert.getStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, "main"),
        frag.getStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, "main"),
    };

    auto createPipelineInternal = []
    {
        GraphicsPipelineCreateInfoPack pipelineCreateInfoPack;
        pipelineCreateInfoPack.createInfo.layout = pipelineLayout_planet;
        pipelineCreateInfoPack.createInfo.renderPass = RenderPassAndFrameBuffersWithDepth().renderPass;
        pipelineCreateInfoPack.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        pipelineCreateInfoPack.viewports.emplace_back(VkViewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(windowSize.width),
            .height = static_cast<float>(windowSize.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f});
        pipelineCreateInfoPack.scissors.emplace_back(VkRect2D{
            .offset = {0, 0},
            .extent = windowSize});

        // turn on depth test
        pipelineCreateInfoPack.depthStencilState.depthTestEnable = VK_TRUE;
        pipelineCreateInfoPack.depthStencilState.depthWriteEnable = VK_TRUE;
        pipelineCreateInfoPack.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

        pipelineCreateInfoPack.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        pipelineCreateInfoPack.colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT});

        // vertex input
        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(SphereVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        pipelineCreateInfoPack.bindingDescriptions.push_back(bindingDescription);

        VkVertexInputAttributeDescription attributeDescriptions[] = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, pos)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SphereVertex, normal)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SphereVertex, texCoord)},
        };
        pipelineCreateInfoPack.attributeDescriptions.assign(std::begin(attributeDescriptions), std::end(attributeDescriptions));

        pipelineCreateInfoPack.shaderStages.assign(std::begin(shaderStages), std::end(shaderStages));
        pipelineCreateInfoPack.updateAllVectorData();

        pipeline_planet.create(pipelineCreateInfoPack);
    };
    auto destroyPipelineInternal = []
    {
        pipeline_planet.~Pipeline();
    };

    // recreate pipeline when swapchain is recreated
    VulkanManager::getManager().registerCreateSwapchainCallback(createPipelineInternal);
    VulkanManager::getManager().registerDestroySwapchainCallback(destroyPipelineInternal);

    createPipelineInternal();
}

void createPipelineLayout()
{
    VkDescriptorSetLayoutBinding sceneBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutBinding planetBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    std::array bindings = {sceneBinding, planetBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    descriptorSetLayout_uniform.create(layoutInfo);

    VkDescriptorSetLayoutBinding textureBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo textureLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &textureBinding,
    };
    descriptorSetLayout_material.create(textureLayoutInfo);

    VkDescriptorSetLayout descriptorSetLayouts[] = {descriptorSetLayout_uniform, descriptorSetLayout_material};
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(std::size(descriptorSetLayouts)),
        .pSetLayouts = descriptorSetLayouts,
        .pushConstantRangeCount = 0,
    };
    pipelineLayout_planet.create(pipelineLayoutCreateInfo);

    VkDescriptorSetLayout uniformSetLayouts[] = {descriptorSetLayout_uniform};
    VkPushConstantRange pickingPushConstant = {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(glm::vec4)};
    VkPipelineLayoutCreateInfo pickingLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = uniformSetLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pickingPushConstant};
    pipelineLayout_picking.create(pickingLayoutInfo);

    VkPushConstantRange outlinePushConstant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(glm::vec4)};
    VkPipelineLayoutCreateInfo outlineLayoutInfo = pickingLayoutInfo;
    outlineLayoutInfo.pPushConstantRanges = &outlinePushConstant;
    outlineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayout_outline.create(outlineLayoutInfo);
}

int main()
{
    if (!GLFWWrapper::initWindowWithVulkan(1280, 720, "Solar System"))
    {
        spdlog::error("Failed to initialize GLFW window with Vulkan");
        return -1;
    }

    const auto &[renderPass, frameBuffers] = RenderPassAndFrameBuffersWithDepth();
    createPipelineLayout();
    createPipeline();
    createPickingResources();
    createPickingReadbackBuffer();
    createPickingPipeline();
    createOutlinePipeline();

    CommandBuffer commandBuffer;
    CommandPool commandPool(VulkanManager::getManager().getGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.allocateBuffers(commandBuffer);

    Semaphore semaphoreImageAvailable;
    Semaphore semaphoreRenderFinished;
    Fence fence;

    Sphere sphere(1.0f, glm::vec3(0.0f));
    Texture2D texture_sun("assets/sun.jpg", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
    Texture2D texture_earth("assets/earth.jpg", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
    Texture2D texture_moon("assets/moon.jpg", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
    VkSamplerCreateInfo samplerInfo = Texture::getDefaultSamplerCreateInfo();
    Sampler planetSampler(samplerInfo);

    // only need one copy of the sphere vertex/index data
    VertexBuffer vertexBuffer_sphere(sizeof(SphereVertex) * sphere.vertices.size());
    vertexBuffer_sphere.transferData(sphere.vertices.data(), sizeof(SphereVertex) * sphere.vertices.size());
    IndexBuffer indexBuffer_sphere(sizeof(uint16_t) * sphere.indices.size());
    indexBuffer_sphere.transferData(sphere.indices.data(), sizeof(uint16_t) * sphere.indices.size());

    // create uniform buffers
    UniformBuffer sceneUniformBuffer(sizeof(SceneUniform));
    const VkDeviceSize planetUniformStride = UniformBuffer::getAlignedSize(sizeof(PlanetUniform));
    UniformBuffer planetUniformBuffer(planetUniformStride * kPlanetCount);
    std::vector<uint8_t> planetUniformHostData(static_cast<size_t>(planetUniformStride * kPlanetCount));

    // create descriptor sets
    std::array<VkDescriptorPoolSize, 3> poolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1},
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1},
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kPlanetCount},
    };
    DescriptorPool descriptorPool(kPlanetCount + 1, poolSizes);
    DescriptorSet descriptorSet_uniform;
    descriptorPool.allocateSets(descriptorSet_uniform, descriptorSetLayout_uniform);

    VkDescriptorBufferInfo sceneBufferInfo = {
        .buffer = sceneUniformBuffer,
        .offset = 0,
        .range = sizeof(SceneUniform)};
    descriptorSet_uniform.write(sceneBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);

    VkDescriptorBufferInfo planetBufferInfo = {
        .buffer = planetUniformBuffer,
        .offset = 0,
        .range = sizeof(PlanetUniform)};
    descriptorSet_uniform.write(planetBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1);

    std::array<DescriptorSet, kPlanetCount> textureDescriptorSets;
    std::array<VkDescriptorSetLayout, kPlanetCount> textureSetLayouts;
    textureSetLayouts.fill(descriptorSetLayout_material);
    descriptorPool.allocateSets(textureDescriptorSets, textureSetLayouts);

    const std::array<const Texture2D *, kPlanetCount> planetTextures = {
        &texture_sun,
        &texture_earth,
        &texture_moon,
    };
    for (size_t i = 0; i < planetTextures.size(); ++i)
    {
        VkDescriptorImageInfo imageInfo = planetTextures[i]->getDescriptorImageInfo(planetSampler);
        textureDescriptorSets[i].write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);
    }

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.02f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    const auto startTime = std::chrono::high_resolution_clock::now();
    auto lastFrameTime = startTime;

    while (!GLFWWrapper::shouldClose())
    {
        // calculate delta time and elapsed time
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        const float elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
        lastFrameTime = currentTime;

        GLFWWrapper::pollEvents();

        // process mouse input for picking
        GLFWwindow *windowHandle = GLFWWrapper::getWindow();
        int mouseState = glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_LEFT);
        if (mouseState == GLFW_PRESS && !mouseLeftPressedLastFrame && !pickRequestPending && !pickReadbackPending)
        {
            double cursorX = 0.0;
            double cursorY = 0.0;
            glfwGetCursorPos(windowHandle, &cursorX, &cursorY);
            const int width = static_cast<int>(windowSize.width);
            const int height = static_cast<int>(windowSize.height);
            if (width > 0 && height > 0)
            {
                const int clampedX = std::clamp(static_cast<int>(cursorX), 0, width - 1);
                const int clampedY = std::clamp(static_cast<int>(cursorY), 0, height - 1);
                pendingPickPixel = {clampedX, clampedY};
                pickRequestPending = true;
            }
        }
        mouseLeftPressedLastFrame = (mouseState == GLFW_PRESS);

        if (GLFWWrapper::isMinimized())
        {
            GLFWWrapper::waitEvents();
            continue;
        }

        // update camera based on input
        camera.processInput(windowHandle, deltaTime);

        // update scene uniforms according to camera
        SceneUniform sceneUniform = {
            .view = camera.getViewMatrix(),
            .proj = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height),
                0.1f,
                200.0f)};
        sceneUniform.proj[1][1] *= -1.0f;   // invert Y for Vulkan
        sceneUniformBuffer.transferData(&sceneUniform, sizeof(SceneUniform));   // upload scene uniform

        // update planet data and upload to uniform buffer
        auto writePlanetUniform = [&](uint32_t index, const PlanetUniform &uniformData)
        {
            std::memcpy(
                planetUniformHostData.data() + static_cast<size_t>(index) * static_cast<size_t>(planetUniformStride),
                &uniformData,
                sizeof(PlanetUniform));
        };
        const glm::vec3 upAxis(0.0f, 1.0f, 0.0f);

        // sun
        PlanetUniform sunUniform = {
            .model = glm::scale(
                glm::rotate(glm::mat4(1.0f), elapsedTime * glm::radians(8.0f), upAxis),
                glm::vec3(2.5f)),
            .color = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f)};
        writePlanetUniform(0, sunUniform);
        // earth
        glm::mat4 earthOrbit = glm::rotate(glm::mat4(1.0f), elapsedTime * glm::radians(15.0f), upAxis) *
                               glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f));
        PlanetUniform earthUniform = {
            .model = earthOrbit *
                     glm::rotate(glm::mat4(1.0f), elapsedTime * glm::radians(120.0f), upAxis) *
                     glm::scale(glm::mat4(1.0f), glm::vec3(0.75f)),
            .color = glm::vec4(0.1f, 0.4f, 1.0f, 1.0f)};
        writePlanetUniform(1, earthUniform);
        // moon
        PlanetUniform moonUniform = {
            .model = earthOrbit *
                     glm::rotate(glm::mat4(1.0f), elapsedTime * glm::radians(60.0f), upAxis) *
                     glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 0.0f)) *
                     glm::scale(glm::mat4(1.0f), glm::vec3(0.27f)),
            .color = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f)};
        writePlanetUniform(2, moonUniform);

        // upload planet uniforms to GPU
        planetUniformBuffer.transferData(
            planetUniformHostData.data(),
            static_cast<VkDeviceSize>(planetUniformHostData.size()));

        const bool hasValidExtent = windowSize.width > 0 && windowSize.height > 0;
        bool runPickingPass = pickRequestPending && hasValidExtent;
        if (runPickingPass)
        {
            const int width = static_cast<int>(windowSize.width);
            const int height = static_cast<int>(windowSize.height);
            activePickPixel = pendingPickPixel;
            activePickPixel.x = std::clamp(activePickPixel.x, 0, width - 1);
            activePickPixel.y = std::clamp(activePickPixel.y, 0, height - 1);
        }

        VulkanManager::getManager().swapImage(semaphoreImageAvailable);
        auto imageIndex = VulkanManager::getManager().getCurrentImageIndex();

        // record command buffer to render the scene
        commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        if (runPickingPass)
        {
            std::array<VkClearValue, 1> pickingClearValues = {};
            pickingClearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            pickingRenderPass.cmdBegin(
                commandBuffer,
                pickingFrameBuffer,
                VkRect2D{
                    .offset = {0, 0},
                    .extent = windowSize},
                pickingClearValues);

            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_picking);

            VkDeviceSize pickingVertexOffset = 0;
            vkCmdBindVertexBuffers(
                commandBuffer,
                0,
                1,
                vertexBuffer_sphere.addressOfBuffer(),
                &pickingVertexOffset);
            vkCmdBindIndexBuffer(
                commandBuffer,
                indexBuffer_sphere,
                0,
                VK_INDEX_TYPE_UINT16);

            for (uint32_t planetIndex = 0; planetIndex < kPlanetCount; ++planetIndex)
            {
                uint32_t dynamicOffset = static_cast<uint32_t>(planetUniformStride * planetIndex);
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout_picking,
                    0,
                    1,
                    descriptorSet_uniform.address(),
                    1,
                    &dynamicOffset);

                const glm::vec4 &pickColor = kPickingColors[planetIndex];
                vkCmdPushConstants(
                    commandBuffer,
                    pipelineLayout_picking,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(glm::vec4),
                    &pickColor);

                vkCmdDrawIndexed(
                    commandBuffer,
                    static_cast<uint32_t>(sphere.indices.size()),
                    1,
                    0,
                    0,
                    0);
            }

            pickingRenderPass.cmdEnd(commandBuffer);

            // impicit barrier at the end of RenderPass to transfer the picking image for copy
            VkBufferImageCopy copyRegion = {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1},
                .imageOffset = {activePickPixel.x, activePickPixel.y, 0},
                .imageExtent = {1, 1, 1}};

            vkCmdCopyImageToBuffer(
                commandBuffer,
                pickingImageMemory.getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                pickingReadbackBuffer.getBuffer(),
                1,
                &copyRegion);

            pickRequestPending = false;
            pickReadbackPending = true;
        }
        renderPass.cmdBegin(
            commandBuffer,
            frameBuffers[imageIndex],
            VkRect2D{
                .offset = {0, 0},
                .extent = windowSize},
            clearValues);

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_planet);

        VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            vertexBuffer_sphere.addressOfBuffer(),
            &vertexOffset);
        vkCmdBindIndexBuffer(
            commandBuffer,
            indexBuffer_sphere,
            0,
            VK_INDEX_TYPE_UINT16);

        // draw each planet with dynamic offset
        for (uint32_t planetIndex = 0; planetIndex < kPlanetCount; ++planetIndex)
        {
            uint32_t dynamicOffset = static_cast<uint32_t>(planetUniformStride * planetIndex);
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_planet,
                0,
                1,
                descriptorSet_uniform.address(),
                1,
                &dynamicOffset);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_planet,
                1,
                1,
                textureDescriptorSets[planetIndex].address(),
                0,
                nullptr);

            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(sphere.indices.size()),
                1,
                0,
                0,
                0);
        }

        // draw outline for selected planet if any
        if (selectedPlanet)
        {
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_outline);

            uint32_t outlineOffset = static_cast<uint32_t>(planetUniformStride * selectedPlanet.value());
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_outline,
                0,
                1,
                descriptorSet_uniform.address(),
                1,
                &outlineOffset);

            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout_outline,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(glm::vec4),
                &kOutlineColorAndScale);

            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(sphere.indices.size()),
                1,
                0,
                0,
                0);
        }

        renderPass.cmdEnd(commandBuffer);
        commandBuffer.end();

        // submit command buffer and execute
        VulkanManager::getManager().submitCommandBufferToGraphicsQueue(
            commandBuffer,
            semaphoreImageAvailable,
            semaphoreRenderFinished,
            fence);

        VulkanManager::getManager().presentImage(semaphoreRenderFinished);

        fence.waitAndReset();

        // process picking result if any
        resolvePickingResult();

        int fps = util::updateFps();
        GLFWWrapper::setWindowTitle(("Solar System - FPS: " + std::to_string(fps)).c_str());
    }

    GLFWWrapper::terminateWindowWithVulkan();
    return 0;
}