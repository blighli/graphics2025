#pragma once
#include "vulkan_manager.hpp"

// Some useful macros for handling Vulkan handles
#define DestroyHandleBy(Func) if (handle) { Func(VulkanManager::getManager().getDevice(), handle, nullptr); handle = VK_NULL_HANDLE; }

#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;

#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }

#define DefineAddressFunction const decltype(handle)* address() const { return &handle; }

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

template<typename Func>
void executeOnce(Func&& func)
{
    static bool executed = false;
    if (executed) return;
    executed = true;
    // keep the value category of func
    std::forward<Func>(func)();
}

template<typename T, typename Func>
T executeOnceWithReturn(Func&& func, T defaultRet = T())
{
    static bool executed = false;
    static T ret = defaultRet;
    if (executed) return ret;
    executed = true;
    ret = std::forward<Func>(func)();
    return ret;
}

#ifdef NDEBUG
    constexpr bool IS_DEBUG = false;
#else
    constexpr bool IS_DEBUG = true;
#endif