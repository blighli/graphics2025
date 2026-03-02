#pragma once

#include "learn_vulkan.hpp"
#include "util/macro.hpp"
#include "util/util.hpp"

namespace vulkan
{
    class ShaderModule
    {
        VkShaderModule handle = VK_NULL_HANDLE;
    public:
        ShaderModule() = default;
        ShaderModule(VkShaderModuleCreateInfo &info) 
        { 
            create(info); 
        }
        ShaderModule(const char* filepath)
        {
            create(filepath);
        }
        ShaderModule(size_t codeSize, const uint32_t* pCode)
        {
            create(codeSize, pCode);
        }
        ShaderModule(ShaderModule &&other) noexcept { MoveHandle }
        ~ShaderModule(){DestroyHandleBy(vkDestroyShaderModule)}

        // getters
        DefineHandleTypeOperator;
        DefineAddressFunction;

        VkPipelineShaderStageCreateInfo getStageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const
        {
            return {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = stage,
                .module = handle,
                .pName = entry,
            };
        }

        VkResult create(VkShaderModuleCreateInfo &info)
        {
            info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            VkResult result = vkCreateShaderModule(VulkanManager::getManager().getDevice(), &info, nullptr, &handle);
            if (result != VK_SUCCESS)
                spdlog::error("Failed to create shader module: {}", static_cast<int32_t>(result));
            return result;
        }
        VkResult create(const char* filepath)
        {
            std::vector<uint32_t> code;
            if (!util::readFileAsBytes(filepath, code))
            {
                spdlog::error("Failed to read shader file: {}", filepath);
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkShaderModuleCreateInfo createInfo = {
                .codeSize = code.size() * 4,    // size in bytes
                .pCode = reinterpret_cast<const uint32_t*>(code.data()),
            };
            return create(createInfo);
        }
        VkResult create(size_t codeSize, const uint32_t* pCode)
        {
            VkShaderModuleCreateInfo createInfo = {
                .codeSize = codeSize,
                .pCode = pCode,
            };
            return create(createInfo);
        }
    };
} // namespace vulkan