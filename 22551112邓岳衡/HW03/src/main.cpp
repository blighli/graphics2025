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
#include "misc/model.hpp"
#include "misc/light.hpp"

#include "util/util.hpp"
#include "util/array_view.hpp"
#include "util/macro.hpp"
#include "util/vkresult_wrapper.hpp"

#include <chrono>
#include <array>
#include <memory>
#include <filesystem>

using namespace vulkan;
using namespace gameworld;

struct SceneUniform
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct LightUniform
{
    alignas(16) glm::vec3 viewPos;
    alignas(16) glm::vec3 ambientColor;
    alignas(16) PointLight pointLights[4];
    alignas(16) DirectionalLight dirLight;
    alignas(16) SpotLight spotLight;
};

constexpr uint32_t kPointLightCount = 2;
DescriptorSetLayout descriptorSetLayout_scene;
DescriptorSetLayout descriptorSetLayout_light;
DescriptorSetLayout descriptorSetLayout_material;
PipelineLayout pipelineLayout_model;
Pipeline pipeline_model;
void createModelPipeline();

std::unique_ptr<FreeCamera> freeCamera = std::make_unique<FreeCamera>();
std::unique_ptr<OrbitCamera> orbitCamera = std::make_unique<OrbitCamera>();
Camera *activeCamera = freeCamera.get();
bool useOrbitCamera = false;

const auto &RenderPassAndFrameBuffersWithDepth()
{
    static const auto &rpwf_depth = easy_vulkan::createRenderPassFBWithDepth();
    return rpwf_depth;
}

void createModelPipeline()
{
    static ShaderModule vert("shaders/spv/model.vert.spv");
    static ShaderModule frag("shaders/spv/model.frag.spv");

    auto createPipelineInternal = []
    {
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            vert.getStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, "main"),
            frag.getStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, "main"),
        };
        VkSpecializationMapEntry pointLightCountEntry = {
            .constantID = 0,
            .offset = 0,
            .size = sizeof(uint32_t),
        };
        uint32_t pointLightCount = kPointLightCount;
        VkSpecializationInfo specializationInfo = {
            .mapEntryCount = 1,
            .pMapEntries = &pointLightCountEntry,
            .dataSize = sizeof(uint32_t),
            .pData = &pointLightCount,
        };
        shaderStages[1].pSpecializationInfo = &specializationInfo;

        GraphicsPipelineCreateInfoPack pipelineCreateInfoPack;
        pipelineCreateInfoPack.createInfo.layout = pipelineLayout_model;
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

        pipelineCreateInfoPack.depthStencilState.depthTestEnable = VK_TRUE;
        pipelineCreateInfoPack.depthStencilState.depthWriteEnable = VK_TRUE;
        pipelineCreateInfoPack.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineCreateInfoPack.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        pipelineCreateInfoPack.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(ModelVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        pipelineCreateInfoPack.bindingDescriptions.push_back(bindingDescription);

        VkVertexInputAttributeDescription attributeDescriptions[] = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ModelVertex, position)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ModelVertex, normal)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ModelVertex, texCoord)},
            {.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ModelVertex, tangent)},
            {.location = 4, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(ModelVertex, bitangent)},
        };
        pipelineCreateInfoPack.attributeDescriptions.assign(std::begin(attributeDescriptions), std::end(attributeDescriptions));

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

        pipelineCreateInfoPack.shaderStages.assign(std::begin(shaderStages), std::end(shaderStages));
        pipelineCreateInfoPack.updateAllVectorData();

        pipeline_model.create(pipelineCreateInfoPack);
    };

    auto destroyPipelineInternal = []
    {
        pipeline_model.~Pipeline();
    };

    VulkanManager::getManager().registerCreateSwapchainCallback(createPipelineInternal);
    VulkanManager::getManager().registerDestroySwapchainCallback(destroyPipelineInternal);

    createPipelineInternal();
}

void createPipelineLayout()
{
    VkDescriptorSetLayoutBinding sceneOnlyBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutCreateInfo sceneLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &sceneOnlyBinding,
    };
    descriptorSetLayout_scene.create(sceneLayoutInfo);

    std::array<VkDescriptorSetLayoutBinding, 3> textureBindings = {
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo textureLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(textureBindings.size()),
        .pBindings = textureBindings.data(),
    };
    descriptorSetLayout_material.create(textureLayoutInfo);

    VkDescriptorSetLayoutBinding lightBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo lightLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &lightBinding,
    };
    descriptorSetLayout_light.create(lightLayoutInfo);

    VkDescriptorSetLayout modelSetLayouts[] = {descriptorSetLayout_scene, descriptorSetLayout_material, descriptorSetLayout_light};
    VkPushConstantRange modelPushConstant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4)};
    VkPipelineLayoutCreateInfo modelPipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(std::size(modelSetLayouts)),
        .pSetLayouts = modelSetLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &modelPushConstant};
    pipelineLayout_model.create(modelPipelineLayoutInfo);
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
    createModelPipeline();

    CommandBuffer commandBuffer;
    CommandPool commandPool(VulkanManager::getManager().getGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandPool.allocateBuffers(commandBuffer);

    Semaphore semaphoreImageAvailable;
    Semaphore semaphoreRenderFinished;
    Fence fence;

    // create uniform buffers
    UniformBuffer sceneUniformBuffer(sizeof(SceneUniform));

    // create light uniform buffer(only one directional light needed)
    LightUniform lightUniformData;
    lightUniformData.viewPos = activeCamera->getPosition();
    lightUniformData.ambientColor = glm::vec3(0.2f);
    lightUniformData.dirLight = DirectionalLight{
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .color = glm::vec3(0.5f),
        .intensity = 2.0f};
    for (uint32_t i = 0; i < 4; ++i)
    {
        lightUniformData.pointLights[i] = PointLight{
            .position = glm::vec3(0.0f),
            .color = glm::vec3(0.0f),
            .intensity = 0.0f,
        };
    }
    lightUniformData.pointLights[0] = PointLight{
        .position = glm::vec3(0.0f, 3.0f, 2.0f),
        .color = glm::vec3(1.0f, 0.8f, 0.6f),
        .intensity = 6.0f,
    };
    lightUniformData.pointLights[1] = PointLight{
        .position = glm::vec3(-3.5f, 2.0f, -1.5f),
        .color = glm::vec3(0.6f, 0.8f, 1.0f),
        .intensity = 4.0f,
    };
    lightUniformData.spotLight = SpotLight{
        .position = glm::vec3(0.0f, 4.5f, 4.0f),
        .direction = glm::vec3(0.0f, -1.0f, -0.5f),
        .color = glm::vec3(1.0f),
        .intensity = 3.0f,
        .cutOff = glm::cos(glm::radians(12.5f)),
        .outerCutOff = glm::cos(glm::radians(17.5f)),
    };
    UniformBuffer lightUniformBuffer(sizeof(LightUniform));
    lightUniformBuffer.transferData(&lightUniformData, sizeof(LightUniform));

    // create descriptor sets
    std::array<VkDescriptorPoolSize, 1> poolSizes = {
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 2},
    };
    DescriptorPool descriptorPool(2, poolSizes);
    DescriptorSet descriptorSet_scene;
    descriptorPool.allocateSets(descriptorSet_scene, descriptorSetLayout_scene);
    DescriptorSet descriptorSet_light;
    descriptorPool.allocateSets(descriptorSet_light, descriptorSetLayout_light);

    VkDescriptorBufferInfo sceneBufferInfo = {
        .buffer = sceneUniformBuffer,
        .offset = 0,
        .range = sizeof(SceneUniform)};
    descriptorSet_scene.write(sceneBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);

    VkDescriptorBufferInfo lightBufferInfo = {
        .buffer = lightUniformBuffer,
        .offset = 0,
        .range = sizeof(LightUniform)};
    descriptorSet_light.write(lightBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);


    Model nanosuitModel;
    {
        ModelCreateInfo modelCreateInfo;
        modelCreateInfo.materialSetLayout = descriptorSetLayout_material;
        modelCreateInfo.albedoInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        modelCreateInfo.albedoFinalFormat = VK_FORMAT_R8G8B8A8_SRGB;
        modelCreateInfo.specularInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        modelCreateInfo.specularFinalFormat = VK_FORMAT_R8G8B8A8_UNORM;
        modelCreateInfo.normalInitialFormat = VK_FORMAT_R8G8B8A8_UNORM;
        modelCreateInfo.normalFinalFormat = VK_FORMAT_R8G8B8A8_UNORM;
        modelCreateInfo.samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;   // seamless texture addressing
        modelCreateInfo.samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        modelCreateInfo.samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        const std::filesystem::path nanosuitPath = "assets/nanosuit_reflection/nanosuit.obj";
        const std::filesystem::path textureDirectory = "assets/nanosuit_reflection";
        if (nanosuitModel.loadFromFile(nanosuitPath, modelCreateInfo, textureDirectory))
        {
            nanosuitModel.setScale(glm::vec3(2.0f));
            nanosuitModel.setTranslation(glm::vec3(0.0f, -1.0f, -2.5f));
        }
        else
        {
            spdlog::warn("Failed to load nanosuit model from {}", nanosuitPath.string());
        }
    }

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.02f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (!GLFWWrapper::shouldClose())
    {
        // calculate delta time and elapsed time
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        GLFWWrapper::pollEvents();

        if (GLFWWrapper::isMinimized())
        {
            GLFWWrapper::waitEvents();
            continue;
        }

        // update camera based on input
        GLFWwindow *windowHandle = GLFWWrapper::getWindow();
        const bool tabPressed = glfwGetKey(windowHandle, GLFW_KEY_TAB) == GLFW_PRESS;
        static bool lastTabPressed = false;
        if (tabPressed && !lastTabPressed)
        {
            useOrbitCamera = !useOrbitCamera;
            activeCamera = useOrbitCamera ? static_cast<Camera *>(orbitCamera.get())
                                          : static_cast<Camera *>(freeCamera.get());
        }
        lastTabPressed = tabPressed;
        activeCamera->processInput(windowHandle, deltaTime);

        // update scene uniforms according to camera
        SceneUniform sceneUniform = {
            .view = activeCamera->getViewMatrix(),
            .proj = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height),
                0.1f,
                200.0f)};
        sceneUniform.proj[1][1] *= -1.0f;   // invert Y for Vulkan
        sceneUniformBuffer.transferData(&sceneUniform, sizeof(SceneUniform));   // upload scene uniform

        lightUniformData.viewPos = activeCamera->getPosition();
        lightUniformBuffer.transferData(&lightUniformData, sizeof(LightUniform));

        VulkanManager::getManager().swapImage(semaphoreImageAvailable);
        auto imageIndex = VulkanManager::getManager().getCurrentImageIndex();

        commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // main render pass
        renderPass.cmdBegin(
            commandBuffer,
            frameBuffers[imageIndex],
            VkRect2D{
                .offset = {0, 0},
                .extent = windowSize},
            clearValues);

        if (nanosuitModel.isReady())
        {
            vkCmdBindPipeline(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_model);

            // bind light descriptor set
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_model,
                2,
                1,
                descriptorSet_light.address(),
                0,
                nullptr);

            ModelDrawContext drawContext = {
                .pipelineLayout = pipelineLayout_model,
                .sceneDescriptorSet = descriptorSet_scene,
                .sceneSetIndex = 0,
                .materialSetIndex = 1,
                .pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT};

            nanosuitModel.recordDraw(commandBuffer, drawContext);
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

        int fps = util::updateFps();
        GLFWWrapper::setWindowTitle(("Solar System - FPS: " + std::to_string(fps)).c_str());
    }

    GLFWWrapper::terminateWindowWithVulkan();
    return 0;
}
