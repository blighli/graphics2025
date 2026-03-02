#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_buffer_base.hpp"
#include "vulkan_image.hpp"

#include "util/macro.hpp"
#include "util/util.hpp"

namespace vulkan
{
    class StagingBuffer
    {
    private:
        // singleton staging buffer for main thread
        static inline class
        {
            StagingBuffer *pointer = create();
            StagingBuffer *create()
            {
                static StagingBuffer stagingBuffer;

                VulkanManager::getManager().registerDestroyDeviceCallback([&]()
                                                                          { stagingBuffer.~StagingBuffer(); });
                return &stagingBuffer;
            }

        public:
            StagingBuffer &get() const
            {
                return *pointer;
            }
        } stagingBuffer_mainThread;

    protected:
        BufferMemory bufferMemory;
        VkDeviceSize memoryUsageSize = 0; // the size of mapped usage in the buffer
        Image aliasedImage;

    public:
        StagingBuffer() = default;
        StagingBuffer(VkDeviceSize size)
        {
            expand(size);
        }

        operator VkBuffer() const
        {
            return bufferMemory.getBuffer();
        }
        const VkBuffer *addressOfBuffer() const
        {
            return bufferMemory.addressOfBuffer();
        }
        VkDeviceSize getAllocationSize() const
        {
            return bufferMemory.getAllocationSize();
        }
        VkImage getAliasedImage() const
        {
            return aliasedImage;
        }

        // get data from the staging buffer
        void retrieveData(void *pDataDst, VkDeviceSize size) const
        {
            bufferMemory.retrieveData(pDataDst, size);
        }

        // if the current allocation size is less than the requested size, reallocate the buffer
        void expand(VkDeviceSize size)
        {
            if (size <= getAllocationSize())
                return;

            release();

            VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};
            bufferMemory.create(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        }

        // mannually release the buffer
        void release()
        {
            bufferMemory.~BufferMemory();
            memoryUsageSize = 0;
        }

        void *mapMemory(VkDeviceSize size)
        {
            expand(size);
            void *pData_dts = nullptr;
            bufferMemory.mapMemory(pData_dts, size);
            memoryUsageSize = size;
            return pData_dts;
        }
        void unmapMemory()
        {
            bufferMemory.unmapMemory(memoryUsageSize);
            memoryUsageSize = 0;
        }

        // write data to the staging buffer
        void bufferData(const void *pDataSrc, VkDeviceSize size)
        {
            expand(size);
            bufferMemory.bufferData(pDataSrc, size);
        }

        [[nodiscard]]
        VkImage createAliasedImage2D(VkFormat format, VkExtent2D extent)
        {
            if (!(util::getFormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
            {
                spdlog::error("The format {} does not support blit source feature.", static_cast<int32_t>(format));
                return VK_NULL_HANDLE;
            }

            // FIXME: current formatInfo differs from EasyVulkan tutorial, which may cause problems
            VkDeviceSize imageDataSize = VkDeviceSize(util::getFormatInfo(format).texel_block_size) * extent.width * extent.height;
            if (imageDataSize > getAllocationSize())
            {
                spdlog::error("Staging buffer is not large enough to create aliased image. Required size: {}, allocated size: {}.", imageDataSize, getAllocationSize());
                return VK_NULL_HANDLE;
            }

            VkImageFormatProperties imageFormatProperties;
            vkGetPhysicalDeviceImageFormatProperties(
                VulkanManager::getManager().getPhysicalDevice(),
                format,
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_TILING_LINEAR,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                0,
                &imageFormatProperties);
            if (extent.width > imageFormatProperties.maxExtent.width ||
                extent.height > imageFormatProperties.maxExtent.height ||
                imageDataSize > imageFormatProperties.maxResourceSize)
            {
                spdlog::error("Requested aliased image size exceeds the device limits.");
                return VK_NULL_HANDLE;
            }

            VkImageCreateInfo imageInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = {extent.width, extent.height, 1},
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_LINEAR,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED};

            aliasedImage.~Image();
            VkResult result = aliasedImage.create(imageInfo);
            if (result != VK_SUCCESS)
            {
                spdlog::error("Failed to create aliased image for staging buffer.");
                return VK_NULL_HANDLE;
            }

            VkImageSubresource subresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .arrayLayer = 0};
            VkSubresourceLayout subresourceLayout;
            vkGetImageSubresourceLayout(
                VulkanManager::getManager().getDevice(),
                aliasedImage,
                &subresource,
                &subresourceLayout);
            
            // GPU may add extra padding to each row, if padding then directly return error
            if (subresourceLayout.size != imageDataSize)
            {
                spdlog::error("Aliased image layout size does not match the expected size, with subresource layout size: {}, expected size: {}.", 
                    subresourceLayout.size, imageDataSize);
                aliasedImage.~Image();
                return VK_NULL_HANDLE;
            }

            aliasedImage.bindMemory(bufferMemory.getMemory());
            return aliasedImage;
        }

        // static functions for main thread staging buffer
        static VkBuffer getStagingBuffer_mainThread()
        {
            return stagingBuffer_mainThread.get();
        }
        static void expandStagingBuffer_mainThread(VkDeviceSize size)
        {
            stagingBuffer_mainThread.get().expand(size);
        }
        static void releaseStagingBuffer_mainThread()
        {
            stagingBuffer_mainThread.get().release();
        }
        static void *mapStagingBuffer_mainThread(VkDeviceSize size)
        {
            return stagingBuffer_mainThread.get().mapMemory(size);
        }
        static void unmapStagingBuffer_mainThread()
        {
            stagingBuffer_mainThread.get().unmapMemory();
        }
        static void bufferDataToStagingBuffer_mainThread(const void *pDataSrc, VkDeviceSize size)
        {
            stagingBuffer_mainThread.get().bufferData(pDataSrc, size);
        }
        static void retrieveDataFromStagingBuffer_mainThread(void *pDataDst, VkDeviceSize size)
        {
            stagingBuffer_mainThread.get().retrieveData(pDataDst, size);
        }
        [[nodiscard]]
        static VkImage createAliasedImage2DInStagingBuffer_mainThread(VkFormat format, VkExtent2D extent)
        {
            return stagingBuffer_mainThread.get().createAliasedImage2D(format, extent);
        }
    };

    class DeviceLocalBuffer
    {
    protected:
        BufferMemory bufferMemory;

    public:
        DeviceLocalBuffer() = default;
        DeviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags usage_without_transfer_dst)
        {
            create(size, usage_without_transfer_dst);
        }

        operator VkBuffer() const { return bufferMemory.getBuffer(); }
        const VkBuffer *addressOfBuffer() const { return bufferMemory.addressOfBuffer(); }
        VkDeviceSize getAllocationSize() const { return bufferMemory.getAllocationSize(); }

        // Update continuous data to the device local buffer
        void transferData(const void *pDataSrc, VkDeviceSize size, VkDeviceSize offset = 0)
        {
            // directly memory map if the buffer is host visible
            if (bufferMemory.getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            {
                bufferMemory.bufferData(pDataSrc, size, offset);
                return;
            }

            // use staging buffer to transfer data
            StagingBuffer::bufferDataToStagingBuffer_mainThread(pDataSrc, size);

            // copy from staging buffer to device local buffer
            auto &commandBuffer = VulkanManager::getHelper().getTransferCommandBuffer();
            commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            VkBufferCopy copyRegion = {
                .srcOffset = 0,
                .dstOffset = offset,
                .size = size};
            vkCmdCopyBuffer(
                commandBuffer,
                StagingBuffer::getStagingBuffer_mainThread(),
                bufferMemory.getBuffer(),
                1,
                &copyRegion);
            commandBuffer.end();

            // submit the command buffer and wait for completion
            VulkanManager::getHelper().executeCommandBufferGraphics(commandBuffer);
        }

        // Update strided data to the device local buffer
        void transferData(
            const void *pDataSrc,
            uint32_t elementCount,
            VkDeviceSize elementSize,
            VkDeviceSize strideSrc,
            VkDeviceSize strideDst,
            VkDeviceSize offsetDst = 0)
        {
            if (bufferMemory.getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            {
                void *pDataDst = nullptr;
                bufferMemory.mapMemory(pDataDst, elementCount * strideDst, offsetDst);

                for (uint32_t i = 0; i < elementCount; i++)
                {
                    std::memcpy(
                        static_cast<uint8_t *>(pDataDst) + i * strideDst,
                        static_cast<const uint8_t *>(pDataSrc) + i * strideSrc,
                        elementSize);
                }

                bufferMemory.unmapMemory(elementCount * strideDst, offsetDst);
                return;
            }

            // use staging buffer to transfer data
            StagingBuffer::bufferDataToStagingBuffer_mainThread(pDataSrc, elementCount * strideSrc);
            auto &commandBuffer = VulkanManager::getHelper().getTransferCommandBuffer();
            commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            std::unique_ptr<VkBufferCopy[]> copyRegions = std::make_unique<VkBufferCopy[]>(elementCount);
            for (uint32_t i = 0; i < elementCount; i++)
            {
                copyRegions[i] = VkBufferCopy{
                    .srcOffset = i * strideSrc,
                    .dstOffset = offsetDst + i * strideDst,
                    .size = elementSize};
            }

            vkCmdCopyBuffer(
                commandBuffer,
                StagingBuffer::getStagingBuffer_mainThread(),
                bufferMemory.getBuffer(),
                elementCount,
                copyRegions.get());

            commandBuffer.end();
            VulkanManager::getHelper().executeCommandBufferGraphics(commandBuffer);
        }

        // Update continuous data with no offset using template
        void transferData(const auto &dataSrc) const
        {
            transferData(&dataSrc, sizeof(dataSrc));
        }

        // CAUTION: This function uses vkCmdUpdateBuffer which has size limitation (usually 65536 bytes).
        void cmdUpdateBuffer(
            VkCommandBuffer commandBuffer,
            const void *pDataSrc,
            VkDeviceSize size,
            VkDeviceSize offset = 0) const
        {
            vkCmdUpdateBuffer(
                commandBuffer,
                bufferMemory.getBuffer(),
                offset,
                size,
                static_cast<const uint32_t *>(pDataSrc));
        }
        void cmdUpdateBuffer(
            VkCommandBuffer commandBuffer,
            const auto &dataSrc) const
        {
            vkCmdUpdateBuffer(
                commandBuffer,
                bufferMemory.getBuffer(),
                0,
                sizeof(dataSrc),
                &dataSrc);
        }

        void create(VkDeviceSize size, VkBufferUsageFlags usage_without_transfer_dst)
        {
            VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = usage_without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

            false ||
                bufferMemory.createBuffer(bufferInfo) ||
                bufferMemory.allocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    bufferMemory.allocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
                bufferMemory.bindMemory();
        }
        void recreate(VkDeviceSize size, VkBufferUsageFlags usage_without_transfer_dst)
        {
            VulkanManager::getManager().waitIdleDevice(); // ensure no commands are using the buffer

            bufferMemory.~BufferMemory();
            create(size, usage_without_transfer_dst);
        }
    };

    // Specialized buffer types
    class VertexBuffer : public DeviceLocalBuffer
    {
    public:
        VertexBuffer() = default;
        VertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
            : DeviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsage) {}
        
        void create(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsage);
        }
        void recreate(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsage);
        }
    };

    class IndexBuffer : public DeviceLocalBuffer
    {
    public:
        IndexBuffer() = default;
        IndexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
            : DeviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsage) {}
        
        void create(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsage);
        }
        void recreate(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsage);
        }
    };

    class UniformBuffer : public DeviceLocalBuffer
    {
    public:
        UniformBuffer() = default;
        UniformBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
            : DeviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsage) {}
        
        void create(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsage);
        }
        void recreate(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::recreate(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsage);
        }

        // dynamic uniform buffer alignment
        static VkDeviceSize getAlignedSize(VkDeviceSize originalSize)
        {
            VkDeviceSize minUboAlignment = VulkanManager::getManager().getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
            if (minUboAlignment == 0)
            {
                return originalSize;
            }
            else
            {
                return (originalSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
            }
        }
    };

    class StorageBuffer : public DeviceLocalBuffer
    {
    public:
        StorageBuffer() = default;
        StorageBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
            : DeviceLocalBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsage) {}
        
        void create(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsage);
        }
        void recreate(VkDeviceSize size, VkBufferUsageFlags otherUsage = 0)
        {
            DeviceLocalBuffer::recreate(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsage);
        }

        // dynamic storage buffer alignment
        static VkDeviceSize getAlignedSize(VkDeviceSize originalSize)
        {
            VkDeviceSize minSboAlignment = VulkanManager::getManager().getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
            if (minSboAlignment == 0)
            {
                return originalSize;
            }
            else
            {
                return (originalSize + minSboAlignment - 1) & ~(minSboAlignment - 1);
            }
        }
    };

} // namespace vulkan
