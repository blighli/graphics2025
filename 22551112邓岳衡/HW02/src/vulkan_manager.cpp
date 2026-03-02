#include "vulkan_manager.hpp"
#include "vulkan_helper.hpp"
#include "util/macro.hpp"

using namespace vulkan;
VulkanManager& VulkanManager::getManager()
{
    static VulkanManager instance;
    return instance;
}

VulkanHelper &VulkanManager::getHelper()
{
    static VulkanHelper helper;
    return helper;
}

VulkanManager::~VulkanManager()
{
    shutdownVulkan();
}

VkResult VulkanManager::useLatestVulkanApiVersion()
{
    // vkEnumerateInstanceVersion only available in Vulkan 1.1 and later
    if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
        return vkEnumerateInstanceVersion(&vkApiVersion);
    return VK_SUCCESS;
}

VkResult VulkanManager::checkInstanceLayersSupport(std::span<const char *> layersToCheck) const
{
    uint32_t layerCount = 0;
    std::vector<VkLayerProperties> availableLayers;
    if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available instance layers: {}", static_cast<int32_t>(result));
        return result;
    }

    if (layerCount > 0)
    {
        availableLayers.resize(layerCount);
        if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); result != VK_SUCCESS)
        {
            spdlog::error("Failed to get available instance layers: {}", static_cast<int32_t>(result));
            return result;
        }

        // Check if requested layers are supported
        for (auto &name : layersToCheck)
        {
            bool found = false;
            for (auto &layer : availableLayers)
                if (!strcmp(name, layer.layerName))
                {
                    found = true;
                    break;
                }
            // If not found, mark the layer as unsupported
            if (!found)
                name = nullptr;
        }
    }
    else
    {
        // No instance layers available
        spdlog::warn("No instance layers available.");
        for (auto &name : layersToCheck)
            name = nullptr;
    }

    return VK_SUCCESS;
}
VkResult VulkanManager::checkInstanceExtensionsSupport(std::span<const char *> extensionsToCheck, const char *layerName) const
{
    if (layerName == nullptr)
    {
        spdlog::error("Layer name is null when checking instance extensions support.");
        return VK_RESULT_MAX_ENUM;
    }

    uint32_t extensionCount = 0;
    std::vector<VkExtensionProperties> availableExtensions;
    if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available instance extensions: {}", static_cast<int32_t>(result));
        return result;
    }

    if (extensionCount > 0)
    {
        availableExtensions.resize(extensionCount);
        if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data()); result != VK_SUCCESS)
        {
            spdlog::error("Failed to get available instance extensions: {}", static_cast<int32_t>(result));
            return result;
        }

        // Check if requested extensions are supported
        for (auto &name : extensionsToCheck)
        {
            bool found = false;
            for (auto &ext : availableExtensions)
                if (!strcmp(name, ext.extensionName))
                {
                    found = true;
                    break;
                }
            // If not found, mark the extension as unsupported
            if (!found)
                name = nullptr;
        }
    }
    else
    {
        // No instance extensions available
        spdlog::warn("No instance extensions available.");
        for (auto &name : extensionsToCheck)
            name = nullptr;
    }

    return VK_SUCCESS;
}
VkResult VulkanManager::checkDeviceExtensionsSupport(std::span<const char *> extensionsToCheck, const char *layerName) const
{
    if (layerName == nullptr)
    {
        spdlog::error("Layer name is null when checking device extensions support.");
        return VK_RESULT_MAX_ENUM;
    }

    uint32_t extensionCount = 0;
    std::vector<VkExtensionProperties> availableExtensions;
    if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available device extensions: {}", static_cast<int32_t>(result));
        return result;
    }

    if (extensionCount > 0)
    {
        availableExtensions.resize(extensionCount);
        if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, availableExtensions.data()); result != VK_SUCCESS)
        {
            spdlog::error("Failed to get available device extensions: {}", static_cast<int32_t>(result));
            return result;
        }

        // Check if requested extensions are supported
        for (auto &name : extensionsToCheck)
        {
            bool found = false;
            for (auto &ext : availableExtensions)
                if (!strcmp(name, ext.extensionName))
                {
                    found = true;
                    break;
                }
            // If not found, mark the extension as unsupported
            if (!found)
                name = nullptr;
        }
    }
    else
    {
        // No device extensions available
        spdlog::warn("No device extensions available.");
        for (auto &name : extensionsToCheck)
            name = nullptr;
    }

    return VK_SUCCESS;
}

// Debug Callback
VkBool32 VulkanManager::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        spdlog::warn("Vulkan Warning: {}", pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        spdlog::error("Vulkan Error: {}", pCallbackData->pMessage);
        break;
    default:
        spdlog::info("Vulkan Unknown: {}", pCallbackData->pMessage);
        break;
    }
    return VK_FALSE;
}

VkResult VulkanManager::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback};

    // get the function pointer to vkCreateDebugUtilsMessengerEXT
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr)
    {
        spdlog::error("Could not load vkCreateDebugUtilsMessengerEXT");
        return VK_RESULT_MAX_ENUM;
    }

    // create the debug messenger
    auto res = func(instance, &createInfo, nullptr, &debugMessenger);
    if (res != VK_SUCCESS)
    {
        spdlog::error("Failed to set up debug messenger: {}", static_cast<int32_t>(res));
    }
    return res;
}

VkResult VulkanManager::getAvailablePhysicalDevices()
{
    uint32_t deviceCount = 0;
    if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available physical devices: {}", static_cast<int32_t>(result));
        return result;
    }

    if (deviceCount == 0)
    {
        spdlog::error("No Vulkan-compatible physical devices found.");
        // Terminate the application
        abort();
    }

    availablePhysicalDevices.resize(deviceCount);
    if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data()); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get available physical devices: {}", static_cast<int32_t>(result));
        return result;
    }

    // Log the names of available physical devices
    for (const auto &device : availablePhysicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        spdlog::info("Found device: {}", deviceProperties.deviceName);
    }

    return VK_SUCCESS;
}

// Assume that we can always get the queue family indices that support all graphics, present, and compute
VkResult VulkanManager::checkAndGetQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0)
    {
        spdlog::error("No queue families found for the physical device.");
        return VK_RESULT_MAX_ENUM;
    }

    std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
    // Find a queue family that supports graphics, present, and compute
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 supportGraphics = queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        VkBool32 supportPresentation = false;
        VkBool32 supportCompute = queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        if (surface)
            if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation))
            {
                spdlog::error("Failed to get physical device surface support: {}", static_cast<int32_t>(result));
                return result;
            }

        // Prefer to use the same queue family for graphics, present, and compute if possible
        if (supportGraphics && supportCompute && (!surface || supportPresentation))
        {
            graphicsQueueFamilyIndex = computeQueueFamilyIndex = i;
            if (surface)
                presentQueueFamilyIndex = i;
            return VK_SUCCESS;
        }
    }
    return VK_RESULT_MAX_ENUM;
}

int VulkanManager::calculateScoreForPhysicalDevice(VkPhysicalDevice device) const
{
    int score = 0;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

    // 1.device type
    switch (deviceProperties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        score += 1000;
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        score += 500;
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        score += 100;
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        score += 50;
        break;
    default:
        break;
    }

    // 2. max compute work group count
    score += deviceProperties.limits.maxComputeWorkGroupCount[0] / 1000;

    // 3. memory size
    VkDeviceSize totalMemorySize = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
        if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            totalMemorySize += memoryProperties.memoryHeaps[i].size;

    score += totalMemorySize / (1024 * 1024 * 100); // 1 point per 100MB

    // 4. features
    // could be more detailed...
    if (deviceFeatures.geometryShader)
        score += 100;
    if (deviceFeatures.tessellationShader)
        score += 100;
    if (deviceFeatures.multiViewport)
        score += 50;
    if (deviceFeatures.samplerAnisotropy)
        score += 50;

    // 5. limitations
    // could be more detailed...
    score += deviceProperties.limits.maxImageDimension2D / 1000;
    score += deviceProperties.limits.maxUniformBufferRange / (1024 * 1024); // 1 point per MB
    score += deviceProperties.limits.maxBoundDescriptorSets * 10;

    spdlog::info("Device: {}, Score: {}", deviceProperties.deviceName, score);

    return score;
}

VkResult VulkanManager::validateAndChoosePhysicalDevice(uint32_t deviceIndex)
{
    static constexpr uint32_t notFound = INT32_MAX; //== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
    static std::vector<uint32_t> queueFamilyIndices(availablePhysicalDevices.size());
    static int maxPhysicalDeviceScore = 0;

    // Check if deviceIndex is valid
    if (queueFamilyIndices[deviceIndex] == notFound)
        return VK_RESULT_MAX_ENUM;

    // if the queue family indices for this device have not been queried yet
    if (queueFamilyIndices[deviceIndex] == VK_QUEUE_FAMILY_IGNORED)
    {
        VkResult result = checkAndGetQueueFamilyIndices(availablePhysicalDevices[deviceIndex]);
        // If GetQueueFamilyIndices(...) returns VK_SUCCESS or VK_RESULT_MAX_ENUM
        // (vkGetPhysicalDeviceSurfaceSupportKHR(...) executed successfully but couldn't find all required queue families),
        // it means we have a conclusion about the required queue family indices, save the result to queueFamilyIndices[deviceIndex]
        if (result)
        {
            if (result == VK_RESULT_MAX_ENUM)
                queueFamilyIndices[deviceIndex] = notFound;
            return result;
        }
        else
            queueFamilyIndices[deviceIndex] = graphicsQueueFamilyIndex;
    }
    // If queue family indices for this device have been queried and found to be valid, use them directly
    else
    {
        graphicsQueueFamilyIndex = computeQueueFamilyIndex = queueFamilyIndices[deviceIndex];
        presentQueueFamilyIndex = surface ? queueFamilyIndices[deviceIndex] : VK_QUEUE_FAMILY_IGNORED;
    }
    // Calculate the score for this physical device
    int score = calculateScoreForPhysicalDevice(availablePhysicalDevices[deviceIndex]);
    if (score > maxPhysicalDeviceScore)
        maxPhysicalDeviceScore = score;
    else if (score < maxPhysicalDeviceScore)
    {
        // If the score is less than the maximum score found so far, mark this device as not found
        queueFamilyIndices[deviceIndex] = notFound;
        return VK_RESULT_MAX_ENUM;
    }
    return VK_SUCCESS;
}

VkResult VulkanManager::selectPhysicalDevice()
{
    if (availablePhysicalDevices.empty())
    {
        spdlog::error("No available physical devices to select from.");
        return VK_RESULT_MAX_ENUM;
    }

    // Try to validate and choose each available physical device
    for (uint32_t i = 0; i < availablePhysicalDevices.size(); i++)
    {
        if (VkResult result = validateAndChoosePhysicalDevice(i); result == VK_SUCCESS)
        {
            physicalDevice = availablePhysicalDevices[i];
        }
        else if (result != VK_RESULT_MAX_ENUM)
        {
            spdlog::error("Failed to validate and choose physical device at index {}: {}", i, static_cast<int32_t>(result));
            return result;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        spdlog::error("No suitable physical device found.");
        return VK_RESULT_MAX_ENUM;
    }

    spdlog::info("Selected physical device: {}", physicalDeviceProperties.deviceName);
    return VK_SUCCESS;
}

VkResult VulkanManager::waitIdleDevice() const
{
    if (device == VK_NULL_HANDLE)
    {
        spdlog::warn("Device is null when waiting for idle.");
        return VK_SUCCESS;
    }
    if (VkResult result = vkDeviceWaitIdle(device); result != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle: {}", static_cast<int32_t>(result));
        // may return VK_ERROR_DEVICE_LOST, in which case the device is lost and cannot be used anymore
        return result;
    }
    return VK_SUCCESS;
}

VkResult VulkanManager::createDevice(VkDeviceCreateFlags flags)
{
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[3] = {
        {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .queueCount = 1,
         .pQueuePriorities = &queuePriority},
        {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .queueCount = 1,
         .pQueuePriorities = &queuePriority},
        {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
         .queueCount = 1,
         .pQueuePriorities = &queuePriority}};

    // Only include the queue create info for the queue families that are actually used
    // Remember that Vulkan doesn't allow duplicate queue family indices in the device creation info
    uint32_t queueCreateInfoCount = 0;
    if (graphicsQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
    {
        queueCreateInfo[queueCreateInfoCount].flags = flags;
        queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = graphicsQueueFamilyIndex;
        queueCreateInfoCount++;
    }
    if (computeQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
        computeQueueFamilyIndex != graphicsQueueFamilyIndex)
    {
        queueCreateInfo[queueCreateInfoCount].flags = flags;
        queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = computeQueueFamilyIndex;
        queueCreateInfoCount++;
    }
    if (presentQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
        presentQueueFamilyIndex != graphicsQueueFamilyIndex &&
        presentQueueFamilyIndex != computeQueueFamilyIndex)
    {
        queueCreateInfo[queueCreateInfoCount].flags = flags;
        queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = presentQueueFamilyIndex;
        queueCreateInfoCount++;
    }

    // Get device features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .flags = flags,
        .queueCreateInfoCount = queueCreateInfoCount,
        .pQueueCreateInfos = queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size()),
        .ppEnabledExtensionNames = enabledDeviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures};

    // Create the logical device
    if (VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device); result != VK_SUCCESS)
    {
        spdlog::error("Failed to create logical device: {}", static_cast<int32_t>(result));
        return result;
    }
    spdlog::info("Logical device created successfully.");

    // get device queues
    if (graphicsQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    if (presentQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    if (computeQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);

    // Get physical device properties and memory properties
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    // Log some information about the physical device
    spdlog::info("Selected Physical Device: {}", physicalDeviceProperties.deviceName);
    spdlog::info("Vulkan API Version: {}.{}.{}",
                 VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
                 VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
                 VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));

    // Execute device created callbacks
    executeCallbacks(onDeviceCreated);
    return VK_SUCCESS;
}

VkResult VulkanManager::recreateDevice(VkDeviceCreateFlags flags)
{
    // Destroy the old device
    if (device != VK_NULL_HANDLE)
    {
        if (VkResult result = waitIdleDevice();
            result != VK_SUCCESS &&
            result != VK_ERROR_DEVICE_LOST) // if device is lost, we can still recreate it
            return result;

        // Cleanup swapchain if exists
        if (swapchain)
        {
            executeCallbacks(onSwapchainDestroyed);
            clearImageView();
            vkDestroySwapchainKHR(device, swapchain, nullptr);

            // Reset swapchain related members
            swapchain = VK_NULL_HANDLE;
            swapchainCreateInfo = {};
        }

        // Execute device destroyed callbacks
        executeCallbacks(onDeviceDestroyed);

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
        computeQueue = VK_NULL_HANDLE;
    }

    // Create the new device
    return createDevice(flags);
}

VkResult VulkanManager::createInstance(VkInstanceCreateFlags flags)
{
    if constexpr (IS_DEBUG)
    {
        // Enable validation layers and debug utils extension in debug mode
        addInstanceLayer("VK_LAYER_KHRONOS_validation");
        addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "LearnVulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vkApiVersion};
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(enabledLayers.size()),
        .ppEnabledLayerNames = enabledLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
        .ppEnabledExtensionNames = enabledExtensions.data()};
    if (VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); result != VK_SUCCESS)
    {
        spdlog::error("Failed to create Vulkan instance: {}", static_cast<int32_t>(result));
        return result;
    };
    spdlog::info("Vulkan instance created successfully.");

    if constexpr (IS_DEBUG)
    {
        // Setup debug messenger here if needed
        createDebugMessenger();
    }

    return VK_SUCCESS;
}

VkResult VulkanManager::getSurfaceFormats()
{
    uint32_t formatCount = 0;
    if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available surface formats: {}", static_cast<int32_t>(result));
        return result;
    }
    if (formatCount == 0)
    {
        spdlog::error("No surface formats found.");
        return VK_RESULT_MAX_ENUM;
    }

    availableSurfaceFormats.resize(formatCount);
    if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableSurfaceFormats.data()); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get available surface formats: {}", static_cast<int32_t>(result));
        return result;
    }

    // spdlog::info("Available Surface Formats:");
    // for (const auto &format : availableSurfaceFormats)
    // {
    //     spdlog::info("Format: {}, Color Space: {}", format.format, format.colorSpace);
    // }
    return VK_SUCCESS;
}

VkResult VulkanManager::setSurfaceFormat(VkSurfaceFormatKHR format)
{
    // Check if the requested format is in the available formats
    bool found = false;
    VkSurfaceFormatKHR selectedFormat = {};
    for (const auto &availableFormat : availableSurfaceFormats)
    {
        // If format.format is 0, it means any format is acceptable
        if ((!format.format || availableFormat.format == format.format) &&
            availableFormat.colorSpace == format.colorSpace)
        {
            selectedFormat = availableFormat;
            found = true;
            break;
        }
    }
    if (!found)
    {
        spdlog::error("Requested surface format not available.");
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    swapchainCreateInfo.imageFormat = selectedFormat.format;
    swapchainCreateInfo.imageColorSpace = selectedFormat.colorSpace;

    // If the swapchain already exists, we need to recreate it
    if (swapchain)
    {
        spdlog::warn("Swapchain already exists. Need to recreate swapchain after changing surface format.");
        return recreateSwapchain();
    }

    spdlog::info("Chosen Surface Format: Format: {}, Color Space: {}", static_cast<int32_t>(selectedFormat.format), static_cast<int32_t>(selectedFormat.colorSpace));
    return VK_SUCCESS;
}

VkResult VulkanManager::createSwapchainInternal()
{
    // create swapchain
    if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain); result != VK_SUCCESS)
    {
        spdlog::error("Failed to create swapchain: {}", static_cast<int32_t>(result));
        return result;
    }

    // query swapchain images
    uint32_t swapchainImageCount = 0;
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of swapchain images: {}", static_cast<int32_t>(result));
        return result;
    }
    swapchainImages.resize(swapchainImageCount);
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get swapchain images: {}", static_cast<int32_t>(result));
        return result;
    }

    // create image views for swapchain images
    swapchainImageViews.resize(swapchainImageCount);
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchainCreateInfo.imageFormat,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        imageViewCreateInfo.image = swapchainImages[i];
        if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]); result != VK_SUCCESS)
        {
            spdlog::error("Failed to create image view for swapchain image {}: {}", i, static_cast<int32_t>(result));
            return result;
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanManager::createSwapchain(bool limitFrameRate, VkSwapchainCreateFlagsKHR flags)
{
    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get surface capabilities: {}", static_cast<int32_t>(result));
        return result;
    }
    // set image count and extent
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.minImageCount < surfaceCapabilities.maxImageCount ? 1 : 0);
    if (surfaceCapabilities.currentExtent.width == -1 ||
        surfaceCapabilities.currentExtent.height == -1)
    {
        // if window surface size is undefined, set to default size
        VkExtent2D extent = {
            glm::clamp(defaultWindowExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            glm::clamp(defaultWindowExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)};
        swapchainCreateInfo.imageExtent = extent;
    }
    else
        swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    // set preTransform
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    // set alphaMode
    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) // prefer to use the window system's alpha mode if supported
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    else
    {
        // otherwise, choose the first supported alpha mode
        for (size_t i = 0; i < 4; i++)
        {
            VkCompositeAlphaFlagBitsKHR alphaMode = static_cast<VkCompositeAlphaFlagBitsKHR>(1 << i);
            if (surfaceCapabilities.supportedCompositeAlpha & alphaMode)
            {
                swapchainCreateInfo.compositeAlpha = alphaMode;
                break;
            }
        }
    }

    // set image usage
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;          // for rendering
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) // for screenshot
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) // for post-processing effects
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    else
        spdlog::warn("Surface does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT. Some post-processing effects may not work.");

    // set image format and color space
    if (!availableSurfaceFormats.size())
    {
        if (VkResult result = getSurfaceFormats(); result != VK_SUCCESS)
            return result;
    }
    if (!swapchainCreateInfo.imageFormat)
    {
        if (setSurfaceFormat({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}) &&
            setSurfaceFormat({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}))
        {
            // If both preferred formats are not available, just use the first available format
            swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
            swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
            spdlog::warn("Preferred surface formats not available. Using first available format: Format: {}, Color Space: {}",
                         static_cast<int32_t>(swapchainCreateInfo.imageFormat), static_cast<int32_t>(swapchainCreateInfo.imageColorSpace));
        }
    }

    // set presentMode
    uint32_t presentModeCount = 0;
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get count of available present modes: {}", static_cast<int32_t>(result));
        return result;
    }
    if (presentModeCount == 0)
    {
        spdlog::error("No present modes found.");
        return VK_RESULT_MAX_ENUM;
    }
    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes.data()); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get available present modes: {}", static_cast<int32_t>(result));
        return result;
    }
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO is guaranteed to be available
    // if not limit frame rate, try to use MAIL
    if (!limitFrameRate)
    {
        for (const auto &mode : availablePresentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // prefer MAILBOX if available
            {
                swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) // otherwise, use IMMEDIATE if available
                swapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // set other fields
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.flags = flags;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // assume graphics and present queue are the same, otherwise manually transfer ownership
    swapchainCreateInfo.clipped = VK_TRUE;

    // create swapchain
    if (VkResult result = createSwapchainInternal(); result != VK_SUCCESS)
    {
        spdlog::error("Failed to create swapchain: {}", static_cast<int32_t>(result));
        return result;
    }

    // call the user-defined callback if exists
    executeCallbacks(onSwapchainCreated);

    return VK_SUCCESS;
}

VkResult VulkanManager::recreateSwapchain()
{
    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities); result != VK_SUCCESS)
    {
        spdlog::error("Failed to get surface capabilities: {}", static_cast<int32_t>(result));
        return result;
    }

    // if window is minimized, currentExtent will be (0,0)
    if (surfaceCapabilities.currentExtent.width == 0 ||
        surfaceCapabilities.currentExtent.height == 0)
    {
        spdlog::error("Current extent is undefined. Need to set the extent manually.");
        return VK_SUBOPTIMAL_KHR;
    }
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.oldSwapchain = swapchain; // set old swapchain for better performance

    // Wait for the swapchain not being acquired before recreating the swapchain
    VkResult result = vkQueueWaitIdle(graphicsQueue);
    if (!result && graphicsQueue != presentQueue)
        result = vkQueueWaitIdle(presentQueue);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device idle before recreating swapchain: {}", static_cast<int32_t>(result));
        return result;
    }

    // call the user-defined callback if exists before destroying the old swapchain resources
    executeCallbacks(onSwapchainDestroyed);

    // Destroy old swapchain related resources
    clearImageView();

    // create new swapchain
    if (VkResult result = createSwapchainInternal(); result != VK_SUCCESS)
    {
        spdlog::error("Failed to recreate swapchain: {}", static_cast<int32_t>(result));
        return result;
    }
    // call the user-defined callback if exists
    executeCallbacks(onSwapchainCreated);

    return VK_SUCCESS;
}

void VulkanManager::clearImageView()
{
    for (auto imageView : swapchainImageViews)
    {
        if (imageView == VK_NULL_HANDLE)
            continue;
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapchainImageViews.clear();
}

void VulkanManager::executeCallbacks(const std::vector<std::function<void()>> &callbacks)
{
    // use index because range-based for loop may cause problem if add/remove callbacks when callback is being executed
    size_t callbacksSize = callbacks.size();
    for (size_t i = 0; i < callbacksSize; i++)
        callbacks[i]();
}

bool VulkanManager::shutdownVulkan()
{
    if (instance == VK_NULL_HANDLE)
        return true;

    // Destroy device and related resources
    if (device != VK_NULL_HANDLE)
    {
        waitIdleDevice();
        // Cleanup swapchain if exists
        if (swapchain)
        {
            executeCallbacks(onSwapchainDestroyed);
            clearImageView();
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
            swapchainCreateInfo = {};
        }

        // Execute device destroyed callbacks
        executeCallbacks(onDeviceDestroyed);

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
        computeQueue = VK_NULL_HANDLE;
    }

    // Destroy surface
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    // Destroy debug messenger
    if constexpr (IS_DEBUG)
    {
        if (debugMessenger != VK_NULL_HANDLE)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
                func(instance, debugMessenger, nullptr);
            debugMessenger = VK_NULL_HANDLE;
        }
    }

    // Destroy instance
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;

    return true;
}

VkResult VulkanManager::swapImage(VkSemaphore semaphore)
{
    // destroy the old swapchain if exists
    if (swapchainCreateInfo.oldSwapchain != VK_NULL_HANDLE &&
        swapchainCreateInfo.oldSwapchain != swapchain)
    {
        vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    }

    // acquire next image
    VkResult result;
    while (true)
    {
        result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &currentImageIndex);
        if (result == VK_SUCCESS)
            return VK_SUCCESS;

        switch (result)
        {
        // if the swapchain is out of date or suboptimal, recreate it
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            spdlog::warn("Swapchain out of date when acquiring next image. Recreating swapchain...");
            if (VkResult recreateResult = recreateSwapchain(); recreateResult != VK_SUCCESS)
                return recreateResult;
            // recreate swapchain succeeded, try to acquire next image again
            break;
        default:
            spdlog::error("Failed to acquire next image: {}", static_cast<int32_t>(result));
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult VulkanManager::presentImage(VkPresentInfoKHR &presentInfo)
{
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    switch (result)
    {
    case VK_SUCCESS:
        return VK_SUCCESS;
    // if the swapchain is out of date or suboptimal, recreate it
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        return recreateSwapchain();
    default:
        spdlog::error("Failed to present image: {}", static_cast<int32_t>(result));
        return result;
    }
}
VkResult VulkanManager::presentImage(VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &currentImageIndex};
    if (waitSemaphore)
    {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
    }
    return presentImage(presentInfo);
}

VkResult VulkanManager::submitCommandBufferToGraphicsQueue(VkSubmitInfo &submitInfo, VkFence fence)
{
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
        spdlog::error("Failed to submit command buffer to graphics queue: {}", static_cast<int32_t>(result));
    return result;
}
VkResult VulkanManager::submitCommandBufferToGraphicsQueue(
    VkCommandBuffer commandBuffer,
    VkSemaphore imageAvailable,
    VkSemaphore renderingIsOver,
    VkFence fence,
    VkPipelineStageFlags waitStage)
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    if(imageAvailable)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailable;
        submitInfo.pWaitDstStageMask = &waitStage;
    }
    if(renderingIsOver)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderingIsOver;
    }
    return submitCommandBufferToGraphicsQueue(submitInfo, fence);
}
VkResult VulkanManager::submitCommandBufferToGraphicsQueue(VkCommandBuffer commandBuffer, VkFence fence)
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    return submitCommandBufferToGraphicsQueue(submitInfo, fence);
}
VkResult VulkanManager::submitCommandBufferToComputeQueue(VkSubmitInfo &submitInfo, VkFence fence)
{
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(computeQueue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
        spdlog::error("Failed to submit command buffer to compute queue: {}", static_cast<int32_t>(result));
    return result;
}
VkResult VulkanManager::submitCommandBufferToComputeQueue(VkCommandBuffer commandBuffer, VkFence fence)
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    return submitCommandBufferToComputeQueue(submitInfo, fence);
}