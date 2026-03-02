#pragma once
#include "learn_vulkan.hpp"
#include "util/macro.hpp"
#include "util/vkresult_wrapper.hpp"

namespace vulkan
{
    class CommandPool;
    class CommandBuffer
    {
        friend class CommandPool;
        VkCommandBuffer handle = VK_NULL_HANDLE;

    public:
        CommandBuffer() = default;
        CommandBuffer(CommandBuffer &&other) noexcept {MoveHandle}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // secondary command buffer
        VkResultWrapper begin(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo &inheritanceInfo) const
        {
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = flags,
                .pInheritanceInfo = &inheritanceInfo};
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to begin command buffer: {}", static_cast<int32_t>(result));
            return result;
        }
        // primary command buffer
        VkResultWrapper begin(VkCommandBufferUsageFlags flags = 0) const
        {
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = flags,
                .pInheritanceInfo = nullptr};
            VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to begin command buffer: {}", static_cast<int32_t>(result));
            return result;
        }
        // end command buffer
        VkResultWrapper end() const
        {
            VkResult result = vkEndCommandBuffer(handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to end command buffer: {}", static_cast<int32_t>(result));
            return result;
        }
    };
}
