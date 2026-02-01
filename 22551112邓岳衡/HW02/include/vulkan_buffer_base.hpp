#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_device_memory.hpp"

#include "util/macro.hpp"

namespace vulkan
{
    class Buffer
    {
        VkBuffer handle = VK_NULL_HANDLE;

    public:
        Buffer() = default;
        Buffer(VkBufferCreateInfo &info) { create(info); }
        Buffer(Buffer &&other) noexcept { MoveHandle }
        ~Buffer(){DestroyHandleBy(vkDestroyBuffer)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkMemoryAllocateInfo getMemoryAllocateInfo(VkMemoryPropertyFlags desiredProperties) const
        {
            VkMemoryAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VulkanManager::getManager().getDevice(), handle, &memRequirements);
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = UINT32_MAX;

            auto &memProperties = VulkanManager::getManager().getPhysicalDeviceMemoryProperties();
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((memRequirements.memoryTypeBits & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties)
                {
                    allocInfo.memoryTypeIndex = i;
                    break;
                }
            }

            return allocInfo;
        }

        VkResult bindMemory(VkDeviceMemory memory, VkDeviceSize offset = 0) const
        {
            VkResult result = vkBindBufferMemory(VulkanManager::getManager().getDevice(), handle, memory, offset);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to bind buffer memory: {}", static_cast<int32_t>(result));

            return result;
        }

        VkResult create(VkBufferCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VkResult result = vkCreateBuffer(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create buffer: {}", static_cast<int32_t>(result));

            return result;
        }
    };

    class BufferMemory : Buffer,
                         DeviceMemory
    {
    public:
        BufferMemory() = default;
        BufferMemory(VkBufferCreateInfo &bufferInfo, VkMemoryPropertyFlags desiredProperties)
        {
            create(bufferInfo, desiredProperties);
        }
        BufferMemory(BufferMemory &&other) noexcept : Buffer(std::move(other)), DeviceMemory(std::move(other))
        {
            DeviceMemory::isBound = other.DeviceMemory::isBound;
            other.DeviceMemory::isBound = false;
        }
        ~BufferMemory() { DeviceMemory::isBound = false; }

        VkBuffer getBuffer() const { return static_cast<const Buffer &>(*this); }
        const VkBuffer *addressOfBuffer() const { return Buffer::address(); }
        VkDeviceMemory getMemory() const { return static_cast<const DeviceMemory &>(*this); }
        const VkDeviceMemory *addressOfMemory() const { return DeviceMemory::address(); }

        bool isBound() const { return DeviceMemory::isBound; }

        using DeviceMemory::bufferData;
        using DeviceMemory::getAllocationSize;
        using DeviceMemory::getMemoryProperties;
        using DeviceMemory::mapMemory;
        using DeviceMemory::retrieveData;
        using DeviceMemory::unmapMemory;

        VkResult createBuffer(VkBufferCreateInfo &bufferInfo)
        {
            return Buffer::create(bufferInfo);
        }
        VkResult allocateMemory(VkMemoryPropertyFlags desiredProperties)
        {
            VkMemoryAllocateInfo allocInfo = Buffer::getMemoryAllocateInfo(desiredProperties);
            if (allocInfo.memoryTypeIndex >= VulkanManager::getManager().getPhysicalDeviceMemoryProperties().memoryTypeCount)
            {
                spdlog::error("Failed to find suitable memory type for buffer.");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            return DeviceMemory::allocate(allocInfo);
        }
        VkResult bindMemory()
        {
            if (VkResult result = Buffer::bindMemory(getMemory()); result != VK_SUCCESS)
            {
                return result;
            }

            DeviceMemory::isBound = true;
            return VK_SUCCESS;
        }

        VkResult create(VkBufferCreateInfo &bufferInfo, VkMemoryPropertyFlags desiredProperties)
        {
            VkResult result;

            false ||
                (result = createBuffer(bufferInfo)) ||
                (result = allocateMemory(desiredProperties)) ||
                (result = bindMemory());

            if (result != VK_SUCCESS)
            {
                spdlog::error("Failed to create BufferMemory.");
            }

            return result;
        }
    };

    class BufferView
    {
        VkBufferView handle = VK_NULL_HANDLE;

    public:
        BufferView() = default;
        BufferView(VkBufferViewCreateInfo &info)
        {
            create(info);
        }
        BufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0)
        {
            create(buffer, format, offset, range);
        }
        BufferView(BufferView &&other) noexcept { MoveHandle }
        ~BufferView(){DestroyHandleBy(vkDestroyBufferView)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkBufferViewCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            VkResult result = vkCreateBufferView(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create buffer view: {}", static_cast<int32_t>(result));

            return result;
        }
        VkResult create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0)
        {
            VkBufferViewCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
                .buffer = buffer,
                .format = format,
                .offset = offset,
                .range = range};
            return create(info);
        }
    };
} // namespace vulkan
