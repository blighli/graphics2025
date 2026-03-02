#pragma once

#include "util/macro.hpp"
#include "util/vkresult_wrapper.hpp"
#include "util/array_view.hpp"
#include "learn_vulkan.hpp"

namespace vulkan
{
    class CommandBuffer;
    class CommandPool
    {
        VkCommandPool handle = VK_NULL_HANDLE;

    public:
        CommandPool() = default;
        CommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) { create(queueFamilyIndex, flags); }
        CommandPool(VkCommandPoolCreateInfo &info) { create(info); }
        CommandPool(CommandPool &&other) noexcept { MoveHandle }
        ~CommandPool(){DestroyHandleBy(vkDestroyCommandPool)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResultWrapper allocateBuffers(ArrayView<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const
        {
            VkCommandBufferAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = handle,
                .level = level,
                .commandBufferCount = static_cast<uint32_t>(buffers.count())};
            VkResult result = vkAllocateCommandBuffers(VulkanManager::getManager().getDevice(), &allocInfo, buffers.pointer());
            if (result != VK_SUCCESS)
                spdlog::error("Failed to allocate command buffers: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResultWrapper allocateBuffers(ArrayView<CommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const
        {
            return allocateBuffers(ArrayView<VkCommandBuffer>(reinterpret_cast<VkCommandBuffer *>(buffers.pointer()), buffers.count()), level);
        }

        void freeBuffers(ArrayView<VkCommandBuffer> buffers) const
        {
            vkFreeCommandBuffers(VulkanManager::getManager().getDevice(), handle, static_cast<uint32_t>(buffers.count()), buffers.pointer());
            // Clear the buffer handles
            memset(buffers.pointer(), 0, sizeof(VkCommandBuffer) * buffers.count());
        }
        void freeBuffers(ArrayView<CommandBuffer> buffers) const
        {
            freeBuffers(ArrayView<VkCommandBuffer>(reinterpret_cast<VkCommandBuffer *>(buffers.pointer()), buffers.count()));
        }

        VkResultWrapper create(VkCommandPoolCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            VkResult result = vkCreateCommandPool(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create command pool: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResultWrapper create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
        {
            VkCommandPoolCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex};
            return create(createInfo);
        }
    };
}
