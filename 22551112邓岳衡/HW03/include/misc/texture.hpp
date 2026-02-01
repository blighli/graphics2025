#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_image.hpp"
#include "util/macro.hpp"
#include "util/util.hpp"
#include "util/image_operation.hpp"

#include <stb/stb_image.h>
#include <vulkan/utility/vk_format_utils.h>
#include <memory>

namespace vulkan
{
    class Texture
    {
    protected:
        ImageView imageView;
        ImageMemory imageMemory;

        Texture() = default;
        void createImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent3D, uint32_t mipLevels, uint32_t arrayLayers, VkImageCreateFlags flags = 0)
        {
            VkImageCreateInfo createInfo = {
                .flags = flags,
                .imageType = imageType,
                .format = format,
                .extent = extent3D,
                .mipLevels = mipLevels,
                .arrayLayers = arrayLayers,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
            imageMemory.create(createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        void createImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevels, uint32_t arrayLayers, VkImageViewCreateFlags flags = 0)
        {
            imageView.create(
                imageMemory.getImage(),
                viewType,
                format,
                {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                 .baseMipLevel = 0,
                 .levelCount = mipLevels,
                 .baseArrayLayer = 0,
                 .layerCount = arrayLayers},
                flags);
        }

        static std::unique_ptr<uint8_t[]> loadFileInternal(
            const auto *address,    // string pointer or address of resource
            size_t fileSize,        // as long as address is of string, this parameter is ignored
            VkFormat desiredFormat, //  desired format
            VkExtent2D &outExtent)
        {
            VKU_FORMAT_INFO formatInfo = util::getFormatInfo(desiredFormat);
            bool isUnormOrSrgb = vkuFormatIsUNORM(desiredFormat) || vkuFormatIsSRGB(desiredFormat); // treat UNORM and SRGB as integer formats since they are stored as integers and only sample as normalized floats in shaders
            bool isInt = vkuFormatIsSampledInt(desiredFormat) || isUnormOrSrgb;
            bool isFloat = vkuFormatIsSampledFloat(desiredFormat) && (!isUnormOrSrgb);
            uint32_t bytePerChannel = formatInfo.texel_block_size / formatInfo.component_count;
#ifndef NDEBUG
            // only support sampled formats with 1, 2 or 4 components
            if (!(isFloat && bytePerChannel == 4) &&
                !(isInt && util::betweenClosed<int32_t>(bytePerChannel, 1, 2)))
            {
                spdlog::error("Texture::loadFileInternal: desired format {} is not a valid sampled format.", static_cast<int32_t>(desiredFormat));
                abort();
            }
#endif
            int &width = reinterpret_cast<int &>(outExtent.width);
            int &height = reinterpret_cast<int &>(outExtent.height);
            int channleCount;
            void *pImageData = nullptr;

            // loaded from file
            if constexpr (std::same_as<decltype(address), const char *> || std::same_as<decltype(address), char *>)
            {
                if (isInt)
                {
                    if (bytePerChannel == 1)
                        pImageData = stbi_load(
                            reinterpret_cast<const char *>(address),
                            &width,
                            &height,
                            &channleCount,
                            formatInfo.component_count);
                    else
                        pImageData = stbi_load_16(
                            reinterpret_cast<const char *>(address),
                            &width,
                            &height,
                            &channleCount,
                            formatInfo.component_count);
                }
                else if (isFloat)
                {
                    pImageData = stbi_loadf(
                        reinterpret_cast<const char *>(address),
                        &width,
                        &height,
                        &channleCount,
                        formatInfo.component_count);
                }

                if (!pImageData)
                    spdlog::error("Texture::loadFileInternal: failed to load from file {}.", address);
            }

            // load from memory
            if constexpr (std::same_as<decltype(address), const uint8_t *> || std::same_as<decltype(address), uint8_t *>)
            {
                if (fileSize > INT32_MAX) // stbi_load_from_memory only accepts int size
                {
                    spdlog::error("Texture::loadFileInternal: file size exceeds INT32_MAX.");
                    stbi_image_free(pImageData);
                    return nullptr;
                }

                if (isInt)
                {
                    if (bytePerChannel == 1)
                        pImageData = stbi_load_from_memory(
                            address,
                            static_cast<int>(fileSize),
                            &width,
                            &height,
                            &channleCount,
                            formatInfo.component_count);
                    else
                        pImageData = stbi_load_16_from_memory(
                            address,
                            static_cast<int>(fileSize),
                            &width,
                            &height,
                            &channleCount,
                            formatInfo.component_count);
                }
                else if (isFloat)
                {
                    pImageData = stbi_loadf_from_memory(
                        address,
                        static_cast<int>(fileSize),
                        &width,
                        &height,
                        &channleCount,
                        formatInfo.component_count);
                }

                if (!pImageData)
                    spdlog::error("Texture::loadFileInternal: failed to load from the given memory address.");
            }

            return std::unique_ptr<uint8_t[]>(static_cast<uint8_t *>(pImageData));
        }

    public:
        VkImageView getImageView() const { return imageView; }
        VkImage getImage() const { return imageMemory.getImage(); }
        const VkImageView *addressOfImageView() const { return imageView.address(); }
        const VkImage *addressOfImage() const { return imageMemory.addressOfImage(); }

        VkDescriptorImageInfo getDescriptorImageInfo(VkSampler sampler) const
        {
            VkDescriptorImageInfo descriptorImageInfo = {
                .sampler = sampler,
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            return descriptorImageInfo;
        }

        [[nodiscard]]
        static std::unique_ptr<uint8_t[]> loadFileFromPath(
            const char *filepath,
            VkFormat desiredFormat,
            VkExtent2D &outExtent)
        {
            return loadFileInternal(filepath, 0, desiredFormat, outExtent);
        }

        [[nodiscard]]
        static std::unique_ptr<uint8_t[]> loadFileFromMemory(
            const uint8_t *pData,
            size_t fileSize,
            VkFormat desiredFormat,
            VkExtent2D &outExtent)
        {
            return loadFileInternal(pData, fileSize, desiredFormat, outExtent);
        }

        static uint32_t calculateMipLevels(VkExtent2D extent)
        {
            return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
        }

        // copy from buffer to image, blit and generate mipmaps
        static void copyBlitAndGanerateMipmap2d(
            VkBuffer srcBuffer,
            VkImage copyDstImage,
            VkImage blitDstImage,
            VkExtent2D extent,
            uint32_t mipLevels = 1,
            uint32_t arrayLayers = 1,
            VkFilter filter = VK_FILTER_LINEAR)
        {
            bool needGenerateMipmap = (mipLevels > 1);
            bool needBlit = copyDstImage != blitDstImage;
            static constexpr util::ImageMemoryBarrierParamPack imbs[2] = {
                // no mips needed, transfer dst to shader read only
                {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                 VK_ACCESS_SHADER_READ_BIT,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                // mips or blit needed, transfer dst to transfer src for blitting
                {VK_PIPELINE_STAGE_TRANSFER_BIT,
                 VK_ACCESS_TRANSFER_READ_BIT,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
            };

            auto &commandBuffer = VulkanManager::getHelper().getTransferCommandBuffer();
            commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            
            // copy from buffer to copyDstImage
            VkBufferImageCopy copyRegion = {
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = arrayLayers},
                .imageExtent = {.width = extent.width, .height = extent.height, .depth = 1}};
            util::cmdCopyBufferToImage(
                commandBuffer,
                srcBuffer,
                copyDstImage,
                copyRegion,
                { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
                imbs[(needGenerateMipmap || needBlit) ? 1 : 0]);

            // blit if needed
            if (needBlit)
            {
                VkImageBlit blitRegion = {
                    .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = arrayLayers},
                    .srcOffsets = {{0, 0, 0}, {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}},
                    .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = arrayLayers},
                    .dstOffsets = {{0, 0, 0}, {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}}};
                util::cmdBlitImage(
                    commandBuffer,
                    copyDstImage,
                    blitDstImage,
                    blitRegion,
                    imbs[1],
                    imbs[needGenerateMipmap ? 1 : 0],
                    filter);
            }

            // generate mipmaps if needed
            if (needGenerateMipmap)
            {
                util::cmdGenerateMipmaps2d(
                    commandBuffer,
                    blitDstImage,
                    extent,
                    mipLevels,
                    arrayLayers,
                    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                    filter);
            }

            commandBuffer.end();
            VulkanManager::getHelper().executeCommandBufferGraphics(commandBuffer);
        }

        // blit and generate mipmaps
        static void blitAndGanerateMipmap2d(
            VkImage srcImage_preinitialized,
            VkImage blitDstImage,
            VkExtent2D extent,
            uint32_t mipLevels = 1,
            uint32_t arrayLayers = 1,
            VkFilter filter = VK_FILTER_LINEAR)
        {
            bool needGenerateMipmap = (mipLevels > 1);
            bool needBlit = srcImage_preinitialized != blitDstImage;
            if (!needGenerateMipmap && !needBlit)
                return;

            auto &commandBuffer = VulkanManager::getHelper().getTransferCommandBuffer();
            commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            static constexpr util::ImageMemoryBarrierParamPack imbs[2] = {
                // no mips needed, transfer dst to shader read only
                {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                 VK_ACCESS_SHADER_READ_BIT,
                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                // mips needed, transfer dst to transfer src for blitting
                {VK_PIPELINE_STAGE_TRANSFER_BIT,
                 VK_ACCESS_TRANSFER_READ_BIT,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
            };

            if (needBlit)
            {
                VkImageMemoryBarrier barrierToTransferSrc = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = srcImage_preinitialized,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = arrayLayers}};
                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrierToTransferSrc);

                // blit from pre-initialized image to blitDstImage
                VkImageBlit blitRegion = {
                    .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = arrayLayers},
                    .srcOffsets = {{0, 0, 0}, {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}},
                    .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = arrayLayers},
                    .dstOffsets = {{0, 0, 0}, {static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}}};
                util::cmdBlitImage(
                    commandBuffer,
                    srcImage_preinitialized,
                    blitDstImage,
                    blitRegion,
                    {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED},
                    imbs[needGenerateMipmap ? 1 : 0],
                    filter);
            }

            if (needGenerateMipmap)
            {
                util::cmdGenerateMipmaps2d(
                    commandBuffer,
                    blitDstImage,
                    extent,
                    mipLevels,
                    arrayLayers,
                    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                    filter);
            }

            commandBuffer.end();
            VulkanManager::getHelper().executeCommandBufferGraphics(commandBuffer);
        }

        static VkSamplerCreateInfo getDefaultSamplerCreateInfo()
        {
            VkSamplerCreateInfo samplerCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .mipLodBias = 0.0f,
                .anisotropyEnable = VK_TRUE,
                .maxAnisotropy = VulkanManager::getManager().getPhysicalDeviceProperties().limits.maxSamplerAnisotropy,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0.0f,
                .maxLod = VK_LOD_CLAMP_NONE,
                .borderColor = {},
                .unnormalizedCoordinates = VK_FALSE};
            return samplerCreateInfo;
        }
    };

    class Texture2D : public Texture
    {
    protected:
        VkExtent2D extent = {0, 0};

        void createInternal(VkFormat initialFormat, VkFormat finalFormat, bool generateMipmaps)
        {
            uint32_t mipLevels = generateMipmaps ? calculateMipLevels(extent) : 1;

            // create image and image view
            createImageMemory(
                VK_IMAGE_TYPE_2D,
                finalFormat,
                {.width = extent.width,
                 .height = extent.height,
                 .depth = 1},
                mipLevels,
                1);
            createImageView(
                VK_IMAGE_VIEW_TYPE_2D,
                finalFormat,
                mipLevels,
                1);

            if (initialFormat == finalFormat)
            {
                copyBlitAndGanerateMipmap2d(
                    StagingBuffer::getStagingBuffer_mainThread(),
                    imageMemory.getImage(),
                    imageMemory.getImage(),
                    extent,
                    mipLevels,
                    1);
            }
            else
            {
                // use aliased image in staging buffer to perform format conversion
                if (VkImage srcImage = StagingBuffer::createAliasedImage2DInStagingBuffer_mainThread(initialFormat, extent); srcImage != VK_NULL_HANDLE)
                    blitAndGanerateMipmap2d(
                        srcImage,
                        imageMemory.getImage(),
                        extent,
                        mipLevels,
                        1);
                else
                {
                    // fallback: create temporary image in device local memory
                    VkImageCreateInfo createInfo = {
                        .imageType = VK_IMAGE_TYPE_2D,
                        .format = initialFormat,
                        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
                    vulkan::ImageMemory tempImage(createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                    copyBlitAndGanerateMipmap2d(
                        StagingBuffer::getStagingBuffer_mainThread(),
                        tempImage.getImage(),
                        imageMemory.getImage(),
                        extent,
                        mipLevels,
                        1);
                }
            }
        }

    public:
        Texture2D() = default;
        Texture2D(const char *filepath, VkFormat initialFormat, VkFormat finalFormat, bool generateMipmaps = true)
        {
            create(filepath, initialFormat, finalFormat, generateMipmaps);
        }
        Texture2D(const uint8_t *pData, VkExtent2D extent, VkFormat initialFormat, VkFormat finalFormat, bool generateMipmaps = true)
        {
            create(pData, extent, initialFormat, finalFormat, generateMipmaps);
        }

        VkExtent2D getExtent() const { return extent; }
        uint32_t width() const { return extent.width; }
        uint32_t height() const { return extent.height; }

        // create from file path
        void create(const char *filepath, VkFormat initialFormat, VkFormat finalFormat, bool generateMipmaps = true)
        {
            VkExtent2D extent;
            VKU_FORMAT_INFO formatInfo = util::getFormatInfo(initialFormat);
            std::unique_ptr<uint8_t[]> imageData = Texture::loadFileFromPath(filepath, initialFormat, extent);
            if (!imageData)
            {
                spdlog::error("Texture2D::create: failed to load image from file: {}.", filepath);
                return;
            }

            create(imageData.get(), extent, initialFormat, finalFormat, generateMipmaps);
        }
        // create from memory data
        void create(const uint8_t *pData, VkExtent2D extent, VkFormat initialFormat, VkFormat finalFormat, bool generateMipmaps = true)
        {
            this->extent = extent;
            size_t imageSize =
                static_cast<size_t>(extent.width) *
                static_cast<size_t>(extent.height) *
                static_cast<size_t>(util::getFormatInfo(initialFormat).texel_block_size);

            StagingBuffer::bufferDataToStagingBuffer_mainThread(pData, imageSize);
            createInternal(initialFormat, finalFormat, generateMipmaps);
        }
    };

} // namespace vulkan
