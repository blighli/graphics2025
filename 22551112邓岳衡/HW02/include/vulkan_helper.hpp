#pragma once
#include "vulkan_manager.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_fence.hpp"

#include <vulkan/utility/vk_format_utils.h>

namespace vulkan
{
    class VulkanHelper
    {
        friend class VulkanManager;

        VkFormatProperties formatProperties[std::size(vku_formats)] = {};
        CommandPool commandPool_graphics;
        CommandPool commandPool_presentation;
        CommandPool commandPool_compute;

        CommandBuffer commandBuffer_transfer;
        CommandBuffer commandBuffer_presentation;

        bool isInitialized = false;
        VulkanHelper()
        {
            auto initialize = [this]()
            {
                if (isInitialized)
                    return;

                // Initialize command pools and buffers
                if (VulkanManager::getManager().getGraphicsQueueFamilyIndex() != VK_QUEUE_FAMILY_IGNORED)
                {
                    commandPool_graphics.create(VulkanManager::getManager().getGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    commandPool_graphics.allocateBuffers(commandBuffer_transfer);
                }
                if (VulkanManager::getManager().getComputeQueueFamilyIndex() != VK_QUEUE_FAMILY_IGNORED)
                {
                    commandPool_compute.create(VulkanManager::getManager().getComputeQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                }
                if (VulkanManager::getManager().getPresentQueueFamilyIndex() != VK_QUEUE_FAMILY_IGNORED &&
                    VulkanManager::getManager().getPresentQueueFamilyIndex() != VulkanManager::getManager().getGraphicsQueueFamilyIndex() &&
                    VulkanManager::getManager().getSwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
                {
                    commandPool_presentation.create(VulkanManager::getManager().getPresentQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    commandPool_presentation.allocateBuffers(commandBuffer_presentation);
                }

                // get format properties
                for (size_t i = 0; i < std::size(formatProperties); i++)
                {
                    vkGetPhysicalDeviceFormatProperties(VulkanManager::getManager().getPhysicalDevice(), VkFormat(i), &formatProperties[i]);
                }

                isInitialized = true;
            };
            auto cleanUp = [this]()
            {
                if (!isInitialized)
                    return;

                commandPool_graphics.~CommandPool();
                commandPool_presentation.~CommandPool();
                commandPool_compute.~CommandPool();

                isInitialized = false;
            };

            VulkanManager::getManager().registerCreateDeviceCallback(initialize);
            VulkanManager::getManager().registerDestroyDeviceCallback(cleanUp);

            // Directly call initialize since device is probably already created
            initialize();
        }
        VulkanHelper(VulkanHelper &&) = delete;
        ~VulkanHelper() = default;

    public:
        const VkFormatProperties &getFormatProperties(VkFormat format) const
        {
            if (!isInitialized)
                spdlog::error("VulkanHelper is not initialized. Cannot get VkFormatProperties.");

            if constexpr (IS_DEBUG)
            {
                if (uint32_t(format) >= std::size(vku_formats))
                {
                    spdlog::error("Requested VkFormatProperties for unsupported format: {}", static_cast<int32_t>(format));
                    abort();
                }
            }

            return formatProperties[static_cast<size_t>(format)];
        }

        const CommandPool &getGraphicsCommandPool() const { return commandPool_graphics; }
        const CommandPool &getPresentationCommandPool() const { return commandPool_presentation; }
        const CommandPool &getComputeCommandPool() const { return commandPool_compute; }
        const CommandBuffer &getTransferCommandBuffer() const { return commandBuffer_transfer; }

        VkResult executeCommandBufferGraphics(VkCommandBuffer commandBuffer) const
        {
            Fence fence;
            VkSubmitInfo submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer};

            VkResult result = VulkanManager::getManager().submitCommandBufferToGraphicsQueue(submitInfo, fence);
            if (!result)
                fence.wait();

            return result;
        }
    };
} // namespace vulkan
