#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"
#include "util/array_view.hpp"

namespace vulkan
{
    class RenderPass
    {
        VkRenderPass handle = VK_NULL_HANDLE;

    public:
        RenderPass() = default;
        RenderPass(VkRenderPassCreateInfo &info) { create(info); }
        RenderPass(RenderPass &&other) noexcept { MoveHandle }
        ~RenderPass(){DestroyHandleBy(vkDestroyRenderPass)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        void cmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo &beginInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const
        {
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = handle;
            vkCmdBeginRenderPass(commandBuffer, &beginInfo, contents);
        }
        void cmdBegin(
            VkCommandBuffer commandBuffer,
            VkFramebuffer framebuffer,
            VkRect2D renderArea,
            ArrayView<const VkClearValue> clearValues = {},
            VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const
        {
            VkRenderPassBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = handle,
                .framebuffer = framebuffer,
                .renderArea = renderArea,
                .clearValueCount = static_cast<uint32_t>(clearValues.count()),
                .pClearValues = clearValues.pointer()};
            vkCmdBeginRenderPass(commandBuffer, &beginInfo, contents);
        }

        void cmdNext(VkCommandBuffer commandBuffer, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const
        {
            vkCmdNextSubpass(commandBuffer, contents);
        }

        void cmdEnd(VkCommandBuffer commandBuffer) const
        {
            vkCmdEndRenderPass(commandBuffer);
        }

        VkResult create(VkRenderPassCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            VkResult result = vkCreateRenderPass(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create render pass: {}", static_cast<int32_t>(result));
            
                return result;
        }
    };
} // namespace vulkan
