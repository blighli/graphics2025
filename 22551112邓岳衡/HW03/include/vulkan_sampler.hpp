#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class Sampler
    {
        VkSampler handle = VK_NULL_HANDLE;
    public:
        Sampler() = default;
        Sampler(VkSamplerCreateInfo &info)
        {
            create(info);
        }
        Sampler(Sampler &&other) noexcept { MoveHandle }
        ~Sampler(){DestroyHandleBy(vkDestroySampler)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkSamplerCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            VkResult result = vkCreateSampler(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create sampler: {}", static_cast<int32_t>(result));

            return result;
        }
    };
} // namespace vulkan
