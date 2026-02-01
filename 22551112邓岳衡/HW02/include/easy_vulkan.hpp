#pragma once
#include "vulkan_manager.hpp"
#include "vulkan_frame_buffer.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_semaphore.hpp"

#include "util/macro.hpp"
#include "util/image_operation.hpp"
#include "misc/texture.hpp"

#include <vector>

const VkExtent2D &windowSize = vulkan::VulkanManager::getManager().getSwapchainCreateInfo().imageExtent;

namespace easy_vulkan
{
    using namespace vulkan;
    struct RenderPassWithFrameBuffer
    {
        RenderPass renderPass;
        std::vector<FrameBuffer> frameBuffers;
    };

    const RenderPassWithFrameBuffer &
    createRenderPassWithFrameBuffer()
    {
        static RenderPassWithFrameBuffer rpwf;

        VkAttachmentDescription colorAttachment = {
            .format = VulkanManager::getManager().getSwapchainCreateInfo().imageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT};

        VkRenderPassCreateInfo renderPassCreateInfo = {
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency};

        rpwf.renderPass.create(renderPassCreateInfo);

        auto CreateFramebuffers = []
        {
            rpwf.frameBuffers.resize(VulkanManager::getManager().getSwapchainImageCount());
            VkFramebufferCreateInfo framebufferCreateInfo = {
                .renderPass = rpwf.renderPass,
                .attachmentCount = 1,
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1};

            for (size_t i = 0; i < VulkanManager::getManager().getSwapchainImageCount(); i++)
            {
                VkImageView attachment = VulkanManager::getManager().getSwapchainImageView(i);
                framebufferCreateInfo.pAttachments = &attachment;
                rpwf.frameBuffers[i].create(framebufferCreateInfo);
            }
        };
        auto DestroyFramebuffers = []
        {
            rpwf.frameBuffers.clear();
        };
        CreateFramebuffers();

        // Only register once
        ExecuteOnce(rpwf);
        VulkanManager::getManager().registerDestroySwapchainCallback(DestroyFramebuffers);
        VulkanManager::getManager().registerCreateSwapchainCallback(CreateFramebuffers);
        return rpwf;
    }

    void createDepthResource(ImageMemory &depthImageMemory, ImageView &depthImageView, VkFormat depthFormat = VK_FORMAT_D32_SFLOAT)
    {
        VkExtent2D extent = VulkanManager::getManager().getSwapchainCreateInfo().imageExtent;

        VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = depthFormat,
            .extent = {extent.width, extent.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        depthImageMemory.create(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = depthImageMemory.getImage(),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depthFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        depthImageView.create(viewInfo);
    }

    const RenderPassWithFrameBuffer &
    createRenderPassFBWithDepth()
    {
        static RenderPassWithFrameBuffer rpwf_depth;
        static ImageMemory depthImageMemory;
        static ImageView depthImageView;

        VkAttachmentDescription colorAttachment = {
            .format = VulkanManager::getManager().getSwapchainCreateInfo().imageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentDescription depthAttachment = {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkAttachmentReference depthAttachmentRef = {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT};

        VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassCreateInfo = {
            .attachmentCount = 2,
            .pAttachments = attachments,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency};

        rpwf_depth.renderPass.create(renderPassCreateInfo);

        auto CreateFramebuffers = []
        {
            // create depth resources
            createDepthResource(depthImageMemory, depthImageView, VK_FORMAT_D32_SFLOAT);

            rpwf_depth.frameBuffers.resize(VulkanManager::getManager().getSwapchainImageCount());
            for (size_t i = 0; i < VulkanManager::getManager().getSwapchainImageCount(); i++)
            {
                VkFramebufferCreateInfo framebufferCreateInfo = {
                    .renderPass = rpwf_depth.renderPass,
                    .attachmentCount = 2,
                    .width = windowSize.width,
                    .height = windowSize.height,
                    .layers = 1};

                VkImageView attachments[2] = {
                    VulkanManager::getManager().getSwapchainImageView(i),
                    depthImageView};
                    
                framebufferCreateInfo.pAttachments = attachments;
                rpwf_depth.frameBuffers[i].create(framebufferCreateInfo);
            }
        };
        auto DestroyFramebuffers = []
        {
            depthImageMemory.~ImageMemory();
            rpwf_depth.frameBuffers.clear();
        };
        CreateFramebuffers();

        // Only register once
        ExecuteOnce(rpwf_depth);
        VulkanManager::getManager().registerDestroySwapchainCallback(DestroyFramebuffers);
        VulkanManager::getManager().registerCreateSwapchainCallback(CreateFramebuffers);
        return rpwf_depth;
    }

    void bootScreen(const char *imagePath, VkFormat desiredFormat)
    {
        VkExtent2D imageExtent;
        std::unique_ptr<uint8_t[]> imageData = Texture::loadFileFromPath(imagePath, desiredFormat, imageExtent);
        if (!imageData)
        {
            spdlog::error("easy_vulkan::bootScreen: Failed to load boot screen image from path: {}", imagePath);
            return;
        }

        // upload image data to GPU using staging buffer
        StagingBuffer::bufferDataToStagingBuffer_mainThread(
            imageData.get(),
            static_cast<VkDeviceSize>(imageExtent.width) * imageExtent.height * util::getFormatInfo(desiredFormat).texel_block_size);

        // create command buffer to copy image data to swapchain image
        Semaphore semaphor_imageIsAvailable;
        Fence fence_transferComplete;
        CommandBuffer commandBuffer;
        VulkanManager::getHelper().getGraphicsCommandPool().allocateBuffers(commandBuffer);

        VulkanManager::getManager().swapImage(semaphor_imageIsAvailable);

        commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkExtent2D swapchainExtent = VulkanManager::getManager().getSwapchainCreateInfo().imageExtent;
        bool blit = (imageExtent.width != swapchainExtent.width) ||
                    (imageExtent.height != swapchainExtent.height) ||
                    (desiredFormat != VulkanManager::getManager().getSwapchainCreateInfo().imageFormat);

        ImageMemory imageMemory; // define here to keep the image alive during command buffer execution
        if (blit)
        {
            VkImage image = StagingBuffer::createAliasedImage2DInStagingBuffer_mainThread(
                desiredFormat,
                imageExtent);

            if (image)
            {
                VkImageMemoryBarrier imageMemoryBarrier = {
                    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    nullptr,
                    0,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_PREINITIALIZED,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &imageMemoryBarrier);
            }
            // failed to create aliased image, use device local image instead
            else
            {
                VkImageCreateInfo imageCreateInfo = {
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = desiredFormat,
                    .extent = {imageExtent.width, imageExtent.height, 1},
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                };

                imageMemory.create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                // copy data from staging buffer to image
                VkBufferImageCopy copyRegion = {
                    .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1},
                    .imageExtent = {imageExtent.width, imageExtent.height, 1}};

                util::cmdCopyBufferToImage(
                    commandBuffer,
                    StagingBuffer::getStagingBuffer_mainThread(),
                    imageMemory.getImage(),
                    copyRegion,
                    {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                     0,
                     VK_IMAGE_LAYOUT_UNDEFINED},
                    {VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_ACCESS_TRANSFER_WRITE_BIT,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});

                image = imageMemory.getImage();
            }

            // blit from image to swapchain image
            VkImageBlit blitRegion = {
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1},
                .srcOffsets = {{0, 0, 0}, {static_cast<int32_t>(imageExtent.width), static_cast<int32_t>(imageExtent.height), 1}},
                .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
                .dstOffsets = {{0, 0, 0}, {static_cast<int32_t>(swapchainExtent.width), static_cast<int32_t>(swapchainExtent.height), 1}}};

            util::cmdBlitImage(
                commandBuffer,
                image,
                VulkanManager::getManager().getSwapchainImage(VulkanManager::getManager().getCurrentImageIndex()),
                blitRegion,
                {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                 0,
                 VK_IMAGE_LAYOUT_UNDEFINED},
                {VK_PIPELINE_STAGE_TRANSFER_BIT,
                 VK_ACCESS_TRANSFER_WRITE_BIT,
                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
                VK_FILTER_LINEAR);
        }
        else
        {
            VkBufferImageCopy region = {
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1},
                .imageExtent = {.width = imageExtent.width, .height = imageExtent.height, .depth = 1}};
            util::cmdCopyBufferToImage(
                commandBuffer,
                StagingBuffer::getStagingBuffer_mainThread(),
                VulkanManager::getManager().getSwapchainImage(VulkanManager::getManager().getCurrentImageIndex()),
                region,
                {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                 0,
                 VK_IMAGE_LAYOUT_UNDEFINED},
                {VK_PIPELINE_STAGE_TRANSFER_BIT,
                 VK_ACCESS_TRANSFER_WRITE_BIT,
                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR});
        }

        commandBuffer.end();

        // submit command buffer
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
        VkSubmitInfo submitInfo = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = semaphor_imageIsAvailable.address(),
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = commandBuffer.address()};
        VulkanManager::getManager().submitCommandBufferToGraphicsQueue(submitInfo, fence_transferComplete);
        fence_transferComplete.waitAndReset();

        VulkanManager::getManager().presentImage();

        // cleanup
        VulkanManager::getHelper().getGraphicsCommandPool().freeBuffers(commandBuffer);
    }
} // namespace easy_vulkan
