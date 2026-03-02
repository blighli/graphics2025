#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_device_memory.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class Image
    {
        VkImage handle = VK_NULL_HANDLE;

    public:
        Image() = default;
        Image(VkImageCreateInfo &info) { create(info); }
        Image(Image &&other) noexcept { MoveHandle }
        ~Image(){DestroyHandleBy(vkDestroyImage)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkMemoryAllocateInfo getMemoryAllocateInfo(VkMemoryPropertyFlags desiredProperties) const
        {
            VkMemoryAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(VulkanManager::getManager().getDevice(), handle, &memRequirements);
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = UINT32_MAX;

            auto GetMemoryTypeIndex = [](uint32_t typeBits, VkMemoryPropertyFlags properties) -> uint32_t
            {
                auto &memProperties = VulkanManager::getManager().getPhysicalDeviceMemoryProperties();
                for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
                {
                    if ((typeBits & (1 << i)) &&
                        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    {
                        return i;
                    }
                }
                return UINT32_MAX;
            };
            allocInfo.memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, desiredProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX &&
                desiredProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                allocInfo.memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, desiredProperties & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }

            return allocInfo;
        }
        VkResult bindMemory(VkDeviceMemory memory, VkDeviceSize offset = 0) const
        {
            VkResult result = vkBindImageMemory(VulkanManager::getManager().getDevice(), handle, memory, offset);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to bind image memory: {}", static_cast<int32_t>(result));

            return result;
        }

        VkResult create(VkImageCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            VkResult result = vkCreateImage(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create image: {}", static_cast<int32_t>(result));

            return result;
        }
    };

    class ImageMemory : Image,
                        DeviceMemory
    {
    public:
        ImageMemory() = default;
        ImageMemory(VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags desiredProperties)
        {
            create(imageInfo, desiredProperties);
        }
        ImageMemory(ImageMemory &&other) noexcept : Image(std::move(other)), DeviceMemory(std::move(other))
        {
            DeviceMemory::isBound = other.DeviceMemory::isBound;
            other.DeviceMemory::isBound = false;
        }
        ~ImageMemory() { DeviceMemory::isBound = false; }

        VkImage getImage() const { return static_cast<const Image &>(*this); }
        const VkImage *addressOfImage() const { return Image::address(); }
        VkDeviceMemory getMemory() const { return static_cast<const DeviceMemory &>(*this); }
        const VkDeviceMemory *addressOfMemory() const { return DeviceMemory::address(); }
        bool isBound() const { return DeviceMemory::isBound; }
        using DeviceMemory::getAllocationSize;
        using DeviceMemory::getMemoryProperties;

        VkResult createImage(VkImageCreateInfo &imageInfo)
        {
            return Image::create(imageInfo);
        }
        VkResult allocateMemory(VkMemoryPropertyFlags desiredProperties)
        {
            VkMemoryAllocateInfo allocInfo = Image::getMemoryAllocateInfo(desiredProperties);
            if (allocInfo.memoryTypeIndex >= VulkanManager::getManager().getPhysicalDeviceMemoryProperties().memoryTypeCount)
            {
                spdlog::error("Failed to find suitable memory type for image.");
                return VK_RESULT_MAX_ENUM;
            }

            return DeviceMemory::allocate(allocInfo);
        }
        VkResult bindMemory()
        {
            if (VkResult result = Image::bindMemory(getMemory()); result != VK_SUCCESS)
            {
                return result;
            }

            DeviceMemory::isBound = true;
            return VK_SUCCESS;
        }

        VkResult create(VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags desiredProperties)
        {
            VkResult result;
            false ||
                (result = createImage(imageInfo)) ||
                (result = allocateMemory(desiredProperties)) ||
                (result = bindMemory());
            
                if (result != VK_SUCCESS)
            {
                spdlog::error("Failed to create ImageMemory.");
            }

            return result;
        }
    };

    class ImageView
    {
        VkImageView handle = VK_NULL_HANDLE;

    public:
        ImageView() = default;
        ImageView(VkImageViewCreateInfo &info)
        {
            create(info);
        }
        ImageView(VkImage image, VkImageViewType viewType, VkFormat format,
                  const VkImageSubresourceRange &subresourceRange, VkImageViewCreateFlags flags = 0)
        {
            create(image, viewType, format, subresourceRange, flags);
        }
        ImageView(ImageView &&other) noexcept { MoveHandle }
        ~ImageView(){DestroyHandleBy(vkDestroyImageView)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkImageViewCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            VkResult result = vkCreateImageView(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create image view: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResult create(VkImage image, VkImageViewType viewType, VkFormat format,
                        const VkImageSubresourceRange &subresourceRange, VkImageViewCreateFlags flags = 0)
        {
            VkImageViewCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .flags = flags,
                .image = image,
                .viewType = viewType,
                .format = format,
                .subresourceRange = subresourceRange};

            return create(info);
        }
    };
} // namespace vulkan