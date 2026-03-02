#pragma once
#include "learn_vulkan.hpp"

#ifdef VK_RESULT_THROW
// A wrapper for VkResult that throws an exception if the result is not VK_SUCCESS
class VkResultWrapper
{
    VkResult res;
public:
    inline static void(*errorHandler)(VkResult) = nullptr;
    VkResultWrapper(VkResult r) : res(r) {}
    VkResultWrapper(VkResultWrapper&&) noexcept = default;
    operator VkResult() 
    {
        VkResult r = res;
        res = VK_SUCCESS; // reset to success after conversion
        return r;
    }

    ~VkResultWrapper() 
    {
        // If the result is not success, call the error handler if set
        if (res != VK_SUCCESS && errorHandler)
            errorHandler(res);
    }
};

#elif defined(VK_RESULT_NODISCARD)
// A wrapper for VkResult that marks the result as [[nodiscard]]
struct [[nodiscard]] VkResultWrapper
{
    VkResult res;
    VkResultWrapper(VkResult r) : res(r) {}
    operator VkResult() const { return res; }
};
#else
// do nothing, just use VkResult directly
using VkResultWrapper = VkResult;
#endif