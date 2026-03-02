#pragma once

#include "learn_vulkan.hpp"

namespace util
{
    // Parameters for image memory barrier in command buffer operations
    struct ImageMemoryBarrierParamPack
    {
        const bool isNeeded = false;                            // if barrier is needed
        const VkPipelineStageFlags stage = 0;                   // pipeline stage (src or dst)
        const VkAccessFlags access = 0;                         // access flags (src or dst)
        const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED; // image layout (new or old)

        constexpr ImageMemoryBarrierParamPack() = default;
        constexpr ImageMemoryBarrierParamPack(
            VkPipelineStageFlags stage,
            VkAccessFlags access,
            VkImageLayout layout)
            : isNeeded(true),
              stage(stage),
              access(access),
              layout(layout)
        {
        }
    };

    void cmdCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        const VkBufferImageCopy &copyRegion,
        ImageMemoryBarrierParamPack imb_from,
        ImageMemoryBarrierParamPack imb_to);

    void cmdBlitImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImage dstImage,
        const VkImageBlit &blitRegion,
        ImageMemoryBarrierParamPack imb_dst_from, // source image barrier is assumed to be handled outside
        ImageMemoryBarrierParamPack imb_dst_to,
        VkFilter filter = VK_FILTER_LINEAR);

    void cmdGenerateMipmaps2d(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkExtent2D extent,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        ImageMemoryBarrierParamPack imb_to,
        VkFilter filter = VK_FILTER_LINEAR);

} // namespace util
