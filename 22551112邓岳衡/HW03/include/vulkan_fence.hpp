#pragma once
#include "learn_vulkan.hpp"
#include "util/vkresult_wrapper.hpp"
#include "util/macro.hpp"
#include "vulkan_manager.hpp"

namespace vulkan
{
    class Fence
    {
        VkFence handle = VK_NULL_HANDLE;

    public:
        Fence(VkFenceCreateFlags flags = 0) { create(flags); }
        Fence(VkFenceCreateInfo &info) { create(info); }
        Fence(Fence &&other) noexcept { MoveHandle }
        ~Fence(){DestroyHandleBy(vkDestroyFence)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        // wait for fence
        VkResultWrapper wait(uint64_t timeout = UINT64_MAX) const
        {
            VkResult result = vkWaitForFences(VulkanManager::getManager().getDevice(), 1, &handle, false, timeout);
            if (result != VK_SUCCESS && result != VK_TIMEOUT)
                spdlog::error("Failed to wait for fence: {}", static_cast<int32_t>(result));
            return result;
        }
        // reset fence
        VkResultWrapper reset() const
        {
            VkResult result = vkResetFences(VulkanManager::getManager().getDevice(), 1, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to reset fence: {}", static_cast<int32_t>(result));
            return result;
        }
        // wait and reset fence
        VkResultWrapper waitAndReset(uint64_t timeout = UINT64_MAX) const
        {
            VkResult result = wait(timeout);
            if (result != VK_SUCCESS)
                return result;
            return reset();
        }
        // get fence status
        VkResultWrapper status() const
        {
            VkResult result = vkGetFenceStatus(VulkanManager::getManager().getDevice(), handle);
            if (result < 0)
                spdlog::error("Failed to get fence status: {}", static_cast<int32_t>(result));
            return result;
        }
        // create fence
        VkResultWrapper create(VkFenceCreateInfo &info)
        {
            VkResult result = vkCreateFence(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create fence: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResultWrapper create(VkFenceCreateFlags flags = 0)
        {
            VkFenceCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = flags};
            return create(info);
        }
    };
}
