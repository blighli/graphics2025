#include "util/image_operation.hpp"

namespace util
{
    void cmdCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        const VkBufferImageCopy &copyRegion,
        ImageMemoryBarrierParamPack imb_from,
        ImageMemoryBarrierParamPack imb_to)
    {
        // used in imb_from firstly and imb_to later
        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = imb_from.access,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = imb_from.layout,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // not transfering queue ownership
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = dstImage,
            .subresourceRange = {
                .aspectMask = copyRegion.imageSubresource.aspectMask,
                .baseMipLevel = copyRegion.imageSubresource.mipLevel,
                .levelCount = 1,
                .baseArrayLayer = copyRegion.imageSubresource.baseArrayLayer,
                .layerCount = copyRegion.imageSubresource.layerCount,
            },
        };

        // barrier before copy
        if (imb_from.isNeeded)
        {
            vkCmdPipelineBarrier(
                commandBuffer,
                imb_from.stage,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        vkCmdCopyBufferToImage(
            commandBuffer,
            srcBuffer,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);

        // barrier after copy
        if (imb_to.isNeeded)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = imb_to.access;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = imb_to.layout;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                imb_to.stage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }
    }

    void cmdBlitImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImage dstImage,
        const VkImageBlit &blitRegion,
        ImageMemoryBarrierParamPack imb_dst_from, // source image barrier is assumed to be handled outside
        ImageMemoryBarrierParamPack imb_dst_to,
        VkFilter filter)
    {
        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = imb_dst_from.access,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = imb_dst_from.layout,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // not transfering queue ownership
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = dstImage,
            .subresourceRange = {
                .aspectMask = blitRegion.dstSubresource.aspectMask,
                .baseMipLevel = blitRegion.dstSubresource.mipLevel,
                .levelCount = 1,
                .baseArrayLayer = blitRegion.dstSubresource.baseArrayLayer,
                .layerCount = blitRegion.dstSubresource.layerCount,
            }};

        if (imb_dst_from.isNeeded)
        {
            vkCmdPipelineBarrier(
                commandBuffer,
                imb_dst_from.stage,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        vkCmdBlitImage(
            commandBuffer,
            srcImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blitRegion,
            filter);

        if (imb_dst_to.isNeeded)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = imb_dst_to.access;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = imb_dst_to.layout;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                imb_dst_to.stage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }
    }

    void cmdGenerateMipmaps2d(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkExtent2D extent,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        ImageMemoryBarrierParamPack imb_to,
        VkFilter filter)
    {
        auto getMipmapExtent = [](VkExtent2D baseExtent, uint32_t mipLevel) -> VkOffset3D
        {
            VkOffset3D extent = {
                .x = std::max(static_cast<int32_t>(baseExtent.width) >> mipLevel, 1),
                .y = std::max(static_cast<int32_t>(baseExtent.height) >> mipLevel, 1),
                .z = 1};

            return extent;
        };

        if (arrayLayers > 1)
        {
            std::unique_ptr<VkImageBlit[]> blitRegions = std::make_unique<VkImageBlit[]>(arrayLayers);
            for (uint32_t miplevel = 1; miplevel < mipLevels; ++miplevel)
            {
                VkOffset3D srcExtent = getMipmapExtent(extent, miplevel - 1);
                VkOffset3D dstExtent = getMipmapExtent(extent, miplevel);
                for (uint32_t layer = 0; layer < arrayLayers; ++layer)
                {
                    blitRegions[layer] = VkImageBlit{
                        .srcSubresource = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .mipLevel = miplevel - 1,
                            .baseArrayLayer = layer,
                            .layerCount = 1},
                        .srcOffsets = {{0, 0, 0}, srcExtent},
                        .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = miplevel, .baseArrayLayer = layer, .layerCount = 1},
                        .dstOffsets = {{0, 0, 0}, dstExtent}};
                }
                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = image,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = miplevel,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = arrayLayers}};
                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
                vkCmdBlitImage(
                    commandBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    arrayLayers, blitRegions.get(), filter);
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }
        }
        else
        {
            // generate mipmaps
            for (uint32_t mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
            {
                VkImageBlit blitRegion = {
                    .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = mipLevel - 1, .baseArrayLayer = 0, .layerCount = arrayLayers},
                    .srcOffsets = {{0, 0, 0}, getMipmapExtent(extent, mipLevel - 1)},
                    .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = mipLevel, .baseArrayLayer = 0, .layerCount = arrayLayers},
                    .dstOffsets = {{0, 0, 0}, getMipmapExtent(extent, mipLevel)}};

                util::cmdBlitImage(
                    commandBuffer,
                    image,
                    image,
                    blitRegion,
                    // src barrier
                    {VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED},
                    // dst barrier
                    {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}, // to transfer src since next mipmap level needs it to blit
                    filter);
            }
        }

        // turn all mips to desired layout
        if (imb_to.isNeeded)
        {
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask = imb_to.access,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = imb_to.layout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = arrayLayers}};

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                imb_to.stage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }
    }
} // namespace util