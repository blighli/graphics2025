#pragma once
#include "glfw_wrapper.hpp"
#include "vulkan_manager.hpp"
#include "vulkan_helper.hpp"

#include <vector>
#include <string>
#include <spdlog/spdlog.h>

namespace util
{
    int updateFps();
    bool readFileAsBytes(const std::string &filename, std::vector<uint32_t> &outBytes);

    constexpr VKU_FORMAT_INFO getFormatInfo(VkFormat format)
    {
        if constexpr (IS_DEBUG)
        {
            if (static_cast<size_t>(format) >= std::size(vku_formats))
            {
                spdlog::error("Requested VKU_FORMAT_INFO for unsupported format: {}", static_cast<int32_t>(format));
                abort();
            }
        }

        return vku_formats[static_cast<size_t>(format)];
    }

    constexpr VkFormat get16BitFloatFormat(VkFormat format_32bitFloat)
    {
        switch (format_32bitFloat)
        {
        case VK_FORMAT_R32_SFLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        default:
            spdlog::error("No corresponding 16-bit float format for given 32-bit float format: {}", static_cast<int32_t>(format_32bitFloat));
            return format_32bitFloat;
        }
    }

    inline const VkFormatProperties &getFormatProperties(VkFormat format)
    {
        return vulkan::VulkanManager::getHelper().getFormatProperties(format);
    }

    template<std::signed_integral T>
    constexpr bool betweenClosed(T value, T min, T max)
    {
        return ((value - min) | (max - value)) >= 0;
    }    
}
