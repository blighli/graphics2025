#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"
#include "util/vkresult_wrapper.hpp"

namespace vulkan
{
    class DeviceMemory
    {
        VkDeviceMemory handle = VK_NULL_HANDLE;
        VkDeviceSize allocationSize = 0;
        VkMemoryPropertyFlags memoryProperties = 0;
        // make sure the adjusted range cover the original range
        VkDeviceSize adjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const
        {
            const VkDeviceSize& nonCoherentAtomSize = VulkanManager::getManager().getPhysicalDeviceProperties().limits.nonCoherentAtomSize;
            VkDeviceSize oldOffset = offset;
            offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
            size = std::min((size + offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
            return oldOffset - offset;
        }
    protected:
        // whether the memory is bound to a buffer or image, used by BufferMemory and ImageMemory
        class {
            friend class BufferMemory;
            friend class ImageMemory;
            bool value = false;
            operator bool() const { return value; }
            void operator=(bool v) { value = v; }
        } isBound;
    public:
        DeviceMemory() = default;
        DeviceMemory(VkMemoryAllocateInfo& info) { allocate(info); }
        DeviceMemory(DeviceMemory&& other) noexcept
        {
            MoveHandle
            allocationSize = other.allocationSize;
            memoryProperties = other.memoryProperties;
            other.allocationSize = 0;
            other.memoryProperties = 0;
        }
        ~DeviceMemory() 
        {
            DestroyHandleBy(vkFreeMemory);
            allocationSize = 0;
            memoryProperties = 0; 
        }

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkDeviceSize getAllocationSize() const { return allocationSize; }
        VkMemoryPropertyFlags getMemoryProperties() const { return memoryProperties; }

        VkResultWrapper mapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const
        {
            VkDeviceSize inverseDelta;
            // Check if the memory is coherent
            if(!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                inverseDelta = adjustNonCoherentMemoryRange(size, offset);
            
            // Map the memory
            if(VkResult result = vkMapMemory(VulkanManager::getManager().getDevice(), handle, offset, size, 0, &pData); result != VK_SUCCESS)
            {
                spdlog::error("Failed to map memory: {}", static_cast<int32_t>(result));
                return result;
            }

            // If the memory is not coherent, we need to invalidate the memory
            if(!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                pData = static_cast<uint8_t*>(pData) + inverseDelta;
                VkMappedMemoryRange range = {
                    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .memory = handle,
                    .offset = offset,
                    .size = size
                };
                if(VkResult result = vkInvalidateMappedMemoryRanges(VulkanManager::getManager().getDevice(), 1, &range); result != VK_SUCCESS)
                {
                    spdlog::error("Failed to invalidate mapped memory ranges: {}", static_cast<int32_t>(result));
                    vkUnmapMemory(VulkanManager::getManager().getDevice(), handle);
                    return result;
                }
            }
            return VK_SUCCESS;
        }
        VkResultWrapper unmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const
        {
            // If the memory is not coherent, we need to flush the memory
            if(!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                adjustNonCoherentMemoryRange(size, offset);
                VkMappedMemoryRange range = {
                    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .memory = handle,
                    .offset = offset,
                    .size = size
                };
                if(VkResult result = vkFlushMappedMemoryRanges(VulkanManager::getManager().getDevice(), 1, &range); result != VK_SUCCESS)
                {
                    spdlog::error("Failed to flush mapped memory ranges: {}", static_cast<int32_t>(result));
                    return result;
                }
            }
            vkUnmapMemory(VulkanManager::getManager().getDevice(), handle);
            return VK_SUCCESS;
        }

        // quick buffer data by map, memcpy, unmap
        VkResultWrapper bufferData(const void* pDataSrc, VkDeviceSize size, VkDeviceSize offset = 0) const
        {
            void* pDataDst;
            if(VkResult result = mapMemory(pDataDst, size, offset); result != VK_SUCCESS)
                return result;
            std::memcpy(pDataDst, pDataSrc, static_cast<size_t>(size));
            return unmapMemory(size, offset);
        }
        VkResultWrapper bufferData(const auto& pDataSrc) const
        {
            return bufferData(&pDataSrc, sizeof(pDataSrc));
        }

        // quick retrieve buffer data by map, memcpy, unmap
        VkResultWrapper retrieveData(void* pDataDst, VkDeviceSize size, VkDeviceSize offset = 0) const
        {
            void* pDataSrc;
            if(VkResult result = mapMemory(pDataSrc, size, offset); result != VK_SUCCESS)
                return result;
            std::memcpy(pDataDst, pDataSrc, static_cast<size_t>(size));
            return unmapMemory(size, offset);
        }

        VkResultWrapper allocate(VkMemoryAllocateInfo& info)
        {
            if (handle != VK_NULL_HANDLE)
            {
                spdlog::warn("Memory already allocated. Freeing existing memory before allocating new memory.");
                vkFreeMemory(VulkanManager::getManager().getDevice(), handle, nullptr);
                handle = VK_NULL_HANDLE;
                allocationSize = 0;
                memoryProperties = 0;
            }

            // Get memory properties
            uint32_t memoryTypeIndex = info.memoryTypeIndex;
            const VkPhysicalDeviceMemoryProperties& memoryPropertiesInfo = VulkanManager::getManager().getPhysicalDeviceMemoryProperties();
            if(memoryTypeIndex >= memoryPropertiesInfo.memoryTypeCount)
            {
                spdlog::error("Invalid memory type index: {}", memoryTypeIndex);
                vkFreeMemory(VulkanManager::getManager().getDevice(), handle, nullptr);
                handle = VK_NULL_HANDLE;
                allocationSize = 0;
                return VK_ERROR_INITIALIZATION_FAILED;
            }
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            if (VkResult result = vkAllocateMemory(VulkanManager::getManager().getDevice(), &info, nullptr, &handle); result != VK_SUCCESS)
            {
                spdlog::error("Failed to allocate memory: {}", static_cast<int32_t>(result));
                handle = VK_NULL_HANDLE;
                allocationSize = 0;
                return result;
            }

            // store allocation size and memory properties
            allocationSize = info.allocationSize;
            memoryProperties = memoryPropertiesInfo.memoryTypes[memoryTypeIndex].propertyFlags;
            return VK_SUCCESS;
        }
    };
}