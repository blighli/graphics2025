#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"

namespace vulkan
{
    class QueryPool
    {
        VkQueryPool handle = VK_NULL_HANDLE;

    public:
        QueryPool() = default;
        QueryPool(VkQueryPoolCreateInfo &info)
        {
            create(info);
        }
        QueryPool(VkQueryType type, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics = 0)
        {
            create(type, queryCount, pipelineStatistics);
        }
        QueryPool(QueryPool &&other) noexcept { MoveHandle }
        ~QueryPool(){DestroyHandleBy(vkDestroyQueryPool)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        void cmdReset(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount) const
        {
            vkCmdResetQueryPool(commandBuffer, handle, firstQueryIndex, queryCount);
        }
        void cmdBegin(VkCommandBuffer commandBuffer, uint32_t queryIndex, VkQueryControlFlags flags = 0) const
        {
            vkCmdBeginQuery(commandBuffer, handle, queryIndex, flags);
        }
        void cmdEnd(VkCommandBuffer commandBuffer, uint32_t queryIndex) const
        {
            vkCmdEndQuery(commandBuffer, handle, queryIndex);
        }
        void cmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex) const
        {
            vkCmdWriteTimestamp(commandBuffer, pipelineStage, handle, queryIndex);
        }
        void cmdCopyResults(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount,
                            VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                            VkQueryResultFlags flags = 0) const
        {
            vkCmdCopyQueryPoolResults(commandBuffer, handle, firstQueryIndex, queryCount,
                                      dstBuffer, dstOffset, stride, flags);
        }

        VkResult getResults(uint32_t firstQueryIndex, uint32_t queryCount, size_t dataSize,
                            void *pDataDst, VkDeviceSize stride, VkQueryResultFlags flags = 0) const
        {
            VkResult result = vkGetQueryPoolResults(VulkanManager::getManager().getDevice(),
                                                    handle, firstQueryIndex, queryCount, dataSize,
                                                    pDataDst, stride, flags);

            if (result != VK_SUCCESS)
            {
                result > 0 ? spdlog::warn("Not all query results are available yet: {}", static_cast<int32_t>(result)) : spdlog::error("Failed to get query pool results: {}", static_cast<int32_t>(result));
            }

            return result;
        }
        void reset(uint32_t firstQueryIndex, uint32_t queryCount) const
        {
            vkResetQueryPoolEXT(VulkanManager::getManager().getDevice(), handle, firstQueryIndex, queryCount);
        }

        VkResult create(VkQueryPoolCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            VkResult result = vkCreateQueryPool(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create query pool: {}", static_cast<int32_t>(result));

            return result;
        }
        VkResult create(VkQueryType type, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics = 0)
        {
            VkQueryPoolCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            info.queryType = type;
            info.queryCount = queryCount;
            info.pipelineStatistics = pipelineStatistics;

            return create(info);
        }
    };

    class OcclusionQueries
    {
    protected:
        QueryPool queryPool;
        std::vector<uint32_t> passingSampleCounts;

    public:
        OcclusionQueries() = default;
        OcclusionQueries(uint32_t capacity)
        {
            create(capacity);
        }

        operator VkQueryPool() const { return queryPool; }
        const VkQueryPool *address() const { return queryPool.address(); }
        uint32_t capacity() const { return static_cast<uint32_t>(passingSampleCounts.size()); }
        uint32_t getPassingSampleCount(uint32_t index) const
        {
            return passingSampleCounts[index];
        }

        void cmdReset(VkCommandBuffer commandBuffer) const
        {
            queryPool.cmdReset(commandBuffer, 0, capacity());
        }
        void cmdBegin(VkCommandBuffer commandBuffer, uint32_t queryIndex, bool precise = false) const
        {
            VkQueryControlFlags flags = precise ? VK_QUERY_CONTROL_PRECISE_BIT : 0;
            queryPool.cmdBegin(commandBuffer, queryIndex, flags);
        }
        void cmdEnd(VkCommandBuffer commandBuffer, uint32_t queryIndex) const
        {
            queryPool.cmdEnd(commandBuffer, queryIndex);
        }
        void cmdCopyResults(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount, // often used in GPU-driven Occlusion Culling
                            VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                            VkQueryResultFlags flags = 0) const
        {
            // ensure the results are available before copying
            flags |= VK_QUERY_RESULT_WAIT_BIT;
            queryPool.cmdCopyResults(commandBuffer, firstQueryIndex, queryCount,
                                     dstBuffer, dstOffset, stride, flags);
        }

        void create(uint32_t capacity)
        {
            VkResult result = queryPool.create(VK_QUERY_TYPE_OCCLUSION, capacity);
            if (result != VK_SUCCESS)
            {
                spdlog::error("Failed to create occlusion query pool with capacity {}: {}", capacity, static_cast<int32_t>(result));
                return;
            }

            passingSampleCounts.resize(capacity);
            passingSampleCounts.shrink_to_fit();
        }
        void recreate(uint32_t capacity)
        {
            VulkanManager::getManager().waitIdleDevice();
            queryPool.~QueryPool();
            create(capacity);
        }

        VkResult getResults()
        {
            return getResults(capacity());
        }
        VkResult getResults(uint32_t queryCount)
        {
            return queryPool.getResults(0, queryCount, queryCount * sizeof(uint32_t),
                                        passingSampleCounts.data(), sizeof(uint32_t));
        }
    };

    class PipelineStatisticsQueries
    {
    protected:
        enum StatisticName
        {
            // Input Assembly
            VertexCount_ia,
            PrimitiveCount_ia,
            // Vertex Shader
            InvocationCount_vs,
            // Geometry Shader
            InvocationCount_gs,
            PrimitiveCount_gs,
            // Clipping
            InvocationCount_clipping,
            PrimitiveCount_clipping,
            // Fragment Shader
            InvocationCount_fs,
            // Tessellation
            PatchCount_tcs,
            InvocationCount_tes,
            // Compute Shader
            InvocationCount_cs,

            StatisticCount
        };

        QueryPool queryPool;
        uint32_t statistics[StatisticCount];

    public:
        PipelineStatisticsQueries()
        {
            create();
        }

        operator VkQueryPool() const { return queryPool; }
        const VkQueryPool *address() const { return queryPool.address(); }
        uint32_t vertexCount_ia() const { return statistics[VertexCount_ia]; }
        uint32_t primitiveCount_ia() const { return statistics[PrimitiveCount_ia]; }
        uint32_t invocationCount_vs() const { return statistics[InvocationCount_vs]; }
        uint32_t invocationCount_gs() const { return statistics[InvocationCount_gs]; }
        uint32_t primitiveCount_gs() const { return statistics[PrimitiveCount_gs]; }
        uint32_t invocationCount_clipping() const { return statistics[InvocationCount_clipping]; }
        uint32_t primitiveCount_clipping() const { return statistics[PrimitiveCount_clipping]; }
        uint32_t invocationCount_fs() const { return statistics[InvocationCount_fs]; }
        uint32_t patchCount_tcs() const { return statistics[PatchCount_tcs]; }
        uint32_t invocationCount_tes() const { return statistics[InvocationCount_tes]; }
        uint32_t invocationCount_cs() const { return statistics[InvocationCount_cs]; }

        void cmdReset(VkCommandBuffer commandBuffer) const
        {
            queryPool.cmdReset(commandBuffer, 0, 1);
        }
        void cmdBegin(VkCommandBuffer commandBuffer) const
        {
            queryPool.cmdBegin(commandBuffer, 0);
        }
        void cmdEnd(VkCommandBuffer commandBuffer) const
        {
            queryPool.cmdEnd(commandBuffer, 0);
        }
        void cmdResetAndBegin(VkCommandBuffer commandBuffer) const
        {
            cmdReset(commandBuffer);
            cmdBegin(commandBuffer);
        }

        void create()
        {
            queryPool.create(VK_QUERY_TYPE_PIPELINE_STATISTICS, 1,
                             VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
                                 VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT);
        }
        VkResult getResults()
        {
            return queryPool.getResults(0, 1, StatisticCount * sizeof(uint32_t),
                                        statistics, sizeof(uint32_t));
        }
    };

    class TimestampQueries
    {
    protected:
        QueryPool queryPool;
        std::vector<uint32_t> timestamps;
    public:
        TimestampQueries() = default;
        TimestampQueries(uint32_t capacity)
        {
            create(capacity);
        }

        operator VkQueryPool() const { return queryPool; }
        const VkQueryPool *address() const { return queryPool.address(); }
        uint32_t capacity() const { return static_cast<uint32_t>(timestamps.size()); }
        uint32_t getTimestamp(uint32_t index) const
        {
            return timestamps[index];
        }
        // get the duration in nanoseconds between two timestamps
        uint32_t getDuration(uint32_t startIndex, uint32_t endIndex) const
        {
            return static_cast<uint32_t>((timestamps[endIndex] - timestamps[startIndex]));
        }
        uint32_t getDuration(uint32_t index) const
        {
            return getDuration(index, index + 1);
        }

        void cmdReset(VkCommandBuffer commandBuffer) const
        {
            queryPool.cmdReset(commandBuffer, 0, capacity());
        }
        void cmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex) const
        {
            queryPool.cmdWriteTimestamp(commandBuffer, pipelineStage, queryIndex);
        }

        void create(uint32_t capacity)
        {
            VkResult result = queryPool.create(VK_QUERY_TYPE_TIMESTAMP, capacity);
            if (result != VK_SUCCESS)
            {
                spdlog::error("Failed to create timestamp query pool with capacity {}: {}", capacity, static_cast<int32_t>(result));
                return;
            }

            timestamps.resize(capacity);
            timestamps.shrink_to_fit();
        }
        void recreate(uint32_t capacity)
        {
            VulkanManager::getManager().waitIdleDevice();
            queryPool.~QueryPool();
            create(capacity);
        }

        VkResult getResults()
        {
            return getResults(capacity());
        }
        VkResult getResults(uint32_t queryCount)
        {
            return queryPool.getResults(0, queryCount, queryCount * sizeof(uint32_t),
                                        timestamps.data(), sizeof(uint32_t));
        }
    };

} // namespace vulkan
