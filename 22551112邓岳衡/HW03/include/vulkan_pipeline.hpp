#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class PipelineLayout
    {
        VkPipelineLayout handle = VK_NULL_HANDLE;
    public:
        PipelineLayout() = default;
        PipelineLayout(VkPipelineLayoutCreateInfo &info) { create(info); }
        PipelineLayout(PipelineLayout &&other) noexcept { MoveHandle }
        ~PipelineLayout(){DestroyHandleBy(vkDestroyPipelineLayout)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkPipelineLayoutCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            VkResult result = vkCreatePipelineLayout(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create pipeline layout: {}", static_cast<int32_t>(result));
            return result;
        }
    };


    class Pipeline
    {
        VkPipeline handle = VK_NULL_HANDLE;
    public:
        Pipeline() = default;
        Pipeline(VkGraphicsPipelineCreateInfo &info) { create(info); }
        Pipeline(VkComputePipelineCreateInfo &info) { create(info); }
        Pipeline(Pipeline &&other) noexcept { MoveHandle }
        ~Pipeline(){DestroyHandleBy(vkDestroyPipeline)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkGraphicsPipelineCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateGraphicsPipelines(VulkanManager::getManager().getDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create graphics pipeline: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResult create(VkComputePipelineCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            VkResult result = vkCreateComputePipelines(VulkanManager::getManager().getDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create compute pipeline: {}", static_cast<int32_t>(result));
            return result;
        }
    }; 

    // A helper struct to hold all pipeline creation info
    struct GraphicsPipelineCreateInfoPack
    {
        VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

        // Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

        // Tessellation
        VkPipelineTessellationStateCreateInfo tessellationState = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

        // Viewport State
        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;
        uint32_t dynamicViewportCount = 0;
        uint32_t dynamicScissorCount = 0;

        // Rasterization State
        VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

        // Multisample State
        VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

        // Depth Stencil State
        VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

        // Color Blend State
        VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

        // Dynamic State
        VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        std::vector<VkDynamicState> dynamicStates;

        // --------------------------------------------------------------------------
        GraphicsPipelineCreateInfoPack()
        {
            setAllState();
            // if pipeline is not derived from another pipeline, set to -1
            createInfo.basePipelineIndex = -1;
        }

        GraphicsPipelineCreateInfoPack(const GraphicsPipelineCreateInfoPack& other) noexcept
            : createInfo(other.createInfo),
              shaderStages(other.shaderStages),
              vertexInputState(other.vertexInputState),
              bindingDescriptions(other.bindingDescriptions),
              attributeDescriptions(other.attributeDescriptions),
              inputAssemblyState(other.inputAssemblyState),
              tessellationState(other.tessellationState),
              viewportState(other.viewportState),
              viewports(other.viewports),
              scissors(other.scissors),
              dynamicViewportCount(other.dynamicViewportCount),
              dynamicScissorCount(other.dynamicScissorCount),
              rasterizationState(other.rasterizationState),
              multisampleState(other.multisampleState),
              depthStencilState(other.depthStencilState),
              colorBlendState(other.colorBlendState),
              colorBlendAttachments(other.colorBlendAttachments),
              dynamicState(other.dynamicState),
              dynamicStates(other.dynamicStates)
        {
            setAllState();
            updateAllVectorData();
        }

        operator VkGraphicsPipelineCreateInfo&()
        {
            updateAllVectorData();
            return createInfo;
        }

        // Set createInfo pointers to state structs
        void updateAllVectorData()
        {
            // Shader Stages
            createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            createInfo.pStages = shaderStages.data();

            // Vertex Input
            vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
            vertexInputState.pVertexBindingDescriptions = bindingDescriptions.data();
            vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

            // Viewport State
            viewportState.viewportCount = dynamicViewportCount ? dynamicViewportCount : static_cast<uint32_t>(viewports.size());
            viewportState.pViewports = dynamicViewportCount ? nullptr : viewports.data();
            viewportState.scissorCount = dynamicScissorCount ? dynamicScissorCount : static_cast<uint32_t>(scissors.size());
            viewportState.pScissors = dynamicScissorCount ? nullptr : scissors.data();

            // Color Blend State
            colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
            colorBlendState.pAttachments = colorBlendAttachments.data();

            // Dynamic State
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();
        }

    private:
        // Set pointers in createInfo to corresponding state structs
        void setAllState()
        {
            createInfo.pVertexInputState = &vertexInputState;
            createInfo.pInputAssemblyState = &inputAssemblyState;
            createInfo.pTessellationState = &tessellationState;
            createInfo.pViewportState = &viewportState;
            createInfo.pRasterizationState = &rasterizationState;
            createInfo.pMultisampleState = &multisampleState;
            createInfo.pDepthStencilState = &depthStencilState;
            createInfo.pColorBlendState = &colorBlendState;
            createInfo.pDynamicState = &dynamicState;
        }
    };


}   // namespace vulkan