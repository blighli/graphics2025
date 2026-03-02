#pragma once
#include "learn_vulkan.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class Semaphore
    {
        VkSemaphore handle = VK_NULL_HANDLE;

    public:
        Semaphore(VkSemaphoreCreateFlags flags = 0) { create(flags); }
        Semaphore(VkSemaphoreCreateInfo &info) { create(info); }
        Semaphore(Semaphore &&other) noexcept { MoveHandle }
        ~Semaphore(){DestroyHandleBy(vkDestroySemaphore)}
        // getters
        DefineHandleTypeOperator
            DefineAddressFunction

            // create
            VkResult create(VkSemaphoreCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkResult result = vkCreateSemaphore(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create semaphore: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResult create(VkSemaphoreCreateFlags flags = 0)
        {
            VkSemaphoreCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .flags = flags};
            return create(info);
        }
    };
}
