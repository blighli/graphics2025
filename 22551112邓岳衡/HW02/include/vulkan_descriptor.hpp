#pragma once

#include "learn_vulkan.hpp"
#include "vulkan_buffer_base.hpp"
#include "util/macro.hpp"
#include "util/array_view.hpp"

namespace vulkan
{
    class DescriptorSetLayout
    {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;

    public:
        DescriptorSetLayout() = default;
        DescriptorSetLayout(VkDescriptorSetLayoutCreateInfo &info)
        {
            create(info);
        }
        DescriptorSetLayout(DescriptorSetLayout &&other) noexcept { MoveHandle }
        ~DescriptorSetLayout(){DestroyHandleBy(vkDestroyDescriptorSetLayout)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult create(VkDescriptorSetLayoutCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            VkResult result = vkCreateDescriptorSetLayout(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create descriptor set layout: {}", static_cast<int32_t>(result));

            return result;
        }
    };

    class DescriptorSet
    {
        friend class DescriptorPool;
        VkDescriptorSet handle = VK_NULL_HANDLE;

    public:
        DescriptorSet() = default;
        DescriptorSet(DescriptorSet &&other) noexcept {MoveHandle}

        DefineHandleTypeOperator;
        DefineAddressFunction;

        void write(
            ArrayView<const VkDescriptorImageInfo> imageInfos,
            VkDescriptorType type, uint32_t binding = 0, uint32_t arrayElement = 0) const
        {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = handle,
                .dstBinding = binding,
                .dstArrayElement = arrayElement,
                .descriptorCount = static_cast<uint32_t>(imageInfos.count()),
                .descriptorType = type,
                .pImageInfo = imageInfos.pointer(),
            };
            update(write);
        }
        void write(
            ArrayView<const VkDescriptorBufferInfo> bufferInfos,
            VkDescriptorType type, uint32_t binding = 0, uint32_t arrayElement = 0) const
        {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = handle,
                .dstBinding = binding,
                .dstArrayElement = arrayElement,
                .descriptorCount = static_cast<uint32_t>(bufferInfos.count()),
                .descriptorType = type,
                .pBufferInfo = bufferInfos.pointer(),
            };
            update(write);
        }
        void write(
            ArrayView<const VkBufferView> texelBufferViews,
            VkDescriptorType type, uint32_t binding = 0, uint32_t arrayElement = 0) const
        {
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = handle,
                .dstBinding = binding,
                .dstArrayElement = arrayElement,
                .descriptorCount = static_cast<uint32_t>(texelBufferViews.count()),
                .descriptorType = type,
                .pTexelBufferView = texelBufferViews.pointer(),
            };
            update(write);
        }
        void write(
            ArrayView<const BufferView> bufferViews,
            VkDescriptorType type, uint32_t binding = 0, uint32_t arrayElement = 0) const
        {
            write({bufferViews[0].address(), bufferViews.count()},
                  type, binding, arrayElement);
        }

        static void update(ArrayView<VkWriteDescriptorSet> writes, ArrayView<VkCopyDescriptorSet> copies = {})
        {
            for (auto &write : writes)
            {
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            }
            for (auto &copy : copies)
            {
                copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
            }

            vkUpdateDescriptorSets(
                VulkanManager::getManager().getDevice(),
                writes.count(),
                writes.pointer(),
                copies.count(),
                copies.pointer());
        }
    };

    class DescriptorPool
    {
        VkDescriptorPool handle = VK_NULL_HANDLE;

    public:
        DescriptorPool() = default;
        DescriptorPool(VkDescriptorPoolCreateInfo &info)
        {
            create(info);
        }
        DescriptorPool(uint32_t maxSetsCount, ArrayView<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0)
        {
            create(maxSetsCount, poolSizes, flags);
        }
        DescriptorPool(DescriptorPool &&other) noexcept { MoveHandle }
        ~DescriptorPool(){DestroyHandleBy(vkDestroyDescriptorPool)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkResult allocateSets(
            ArrayView<VkDescriptorSet> outDescriptorSets,
            ArrayView<const VkDescriptorSetLayout> setLayouts) const
        {
            if (outDescriptorSets.count() != setLayouts.count())
            {
                if (outDescriptorSets.count() < setLayouts.count())
                {
                    spdlog::error("Output descriptor sets count is less than set layouts count.");
                    return VK_RESULT_MAX_ENUM;
                }
                else
                    spdlog::warn("Output descriptor sets count is greater than set layouts count. Extra descriptor sets will be ignored.");
            }

            VkDescriptorSetAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = handle,
                .descriptorSetCount = static_cast<uint32_t>(setLayouts.count()),
                .pSetLayouts = setLayouts.pointer(),
            };
            VkResult result = vkAllocateDescriptorSets(
                VulkanManager::getManager().getDevice(),
                &allocInfo,
                outDescriptorSets.pointer());

            if (result != VK_SUCCESS)
                spdlog::error("Failed to allocate descriptor sets: {}", static_cast<int32_t>(result));
            return result;
        }
        // since VkDescriptorSetLayout and DescriptorSetLayout are layout-compatible, provide overloads for convenience
        VkResult allocateSets(ArrayView<VkDescriptorSet> outDescriptorSets, ArrayView<const DescriptorSetLayout> setLayouts) const
        {
            return allocateSets(
                outDescriptorSets,
                {setLayouts[0].address(), setLayouts.count()});
        }
        // overloads for DescriptorSet output
        VkResult allocateSets(
            ArrayView<DescriptorSet> outDescriptorSets,
            ArrayView<const VkDescriptorSetLayout> setLayouts) const
        {
            return allocateSets(
                {&outDescriptorSets[0].handle, outDescriptorSets.count()},
                setLayouts);
        }
        VkResult allocateSets(
            ArrayView<DescriptorSet> outDescriptorSets,
            ArrayView<const DescriptorSetLayout> setLayouts) const
        {
            return allocateSets(
                {&outDescriptorSets[0].handle, outDescriptorSets.count()},
                {setLayouts[0].address(), setLayouts.count()});
        }

        VkResult freeSets(ArrayView<const VkDescriptorSet> descriptorSets) const
        {
            VkResult result = vkFreeDescriptorSets(
                VulkanManager::getManager().getDevice(),
                handle,
                static_cast<uint32_t>(descriptorSets.count()),
                descriptorSets.pointer());
            if (result != VK_SUCCESS)
                spdlog::error("Failed to free descriptor sets: {}", static_cast<int32_t>(result));

            return result;
        }
        VkResult freeSets(ArrayView<const DescriptorSet> descriptorSets) const
        {
            return freeSets({&descriptorSets[0].handle, descriptorSets.count()});
        }

        VkResult create(VkDescriptorPoolCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            VkResult result = vkCreateDescriptorPool(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create descriptor pool: {}", static_cast<int32_t>(result));

            return result;
        }
        VkResult create(uint32_t maxSetsCount, ArrayView<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0)
        {
            VkDescriptorPoolCreateInfo info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = flags,
                .maxSets = maxSetsCount,
                .poolSizeCount = static_cast<uint32_t>(poolSizes.count()),
                .pPoolSizes = poolSizes.pointer(),
            };
            return create(info);
        }
    };
} // namespace vulkan
