#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class FrameBuffer
    {
        VkFramebuffer handle = VK_NULL_HANDLE;
    public:
        FrameBuffer() = default;
        FrameBuffer(VkFramebufferCreateInfo &info) { create(info); }
        FrameBuffer(FrameBuffer &&other) noexcept { MoveHandle }
        ~FrameBuffer(){DestroyHandleBy(vkDestroyFramebuffer)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkFramebufferCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            VkResult result = vkCreateFramebuffer(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create framebuffer: {}", static_cast<int32_t>(result));
            return result;
        }
    };
}   // namespace vulkan