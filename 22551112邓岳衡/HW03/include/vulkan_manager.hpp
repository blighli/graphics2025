#pragma once
#include "learn_vulkan.hpp"
#include <span>

namespace vulkan
{
    class VulkanHelper;
    // Singleton class to manage Vulkan instance, devices
    class VulkanManager
    {
    public:
        static VulkanManager& getManager();
        static constexpr VkExtent2D defaultWindowExtent = {1280, 720};

        // Helper
        static VulkanHelper& getHelper();

        // shut down and cleanup
        bool shutdownVulkan();

        // VkInstance and VkPhysicalDevice
        VkResult useLatestVulkanApiVersion();
        VkResult createInstance(VkInstanceCreateFlags flags = 0);
        uint32_t getVulkanApiVersion() const { return vkApiVersion; }
        VkInstance getInstance() const { return instance; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkResult getAvailablePhysicalDevices();
        int calculateScoreForPhysicalDevice(VkPhysicalDevice device) const;
        VkResult selectPhysicalDevice();
        VkResult validateAndChoosePhysicalDevice(uint32_t deviceIndex = 0);
        const VkPhysicalDeviceProperties &getPhysicalDeviceProperties() const { return physicalDeviceProperties; }
        const VkPhysicalDeviceMemoryProperties &getPhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties; }
        const VkPhysicalDevice &getAvailablePhysicalDevices(size_t index) const { return availablePhysicalDevices[index]; }
        uint32_t getAvailablePhysicalDeviceCount() const { return static_cast<uint32_t>(availablePhysicalDevices.size()); }

        // VkDevice and VkQueue
        VkResult createDevice(VkDeviceCreateFlags flags = 0);
        VkResult recreateDevice(VkDeviceCreateFlags flags = 0);
        VkDevice getDevice() const { return device; }
        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }
        VkQueue getPresentQueue() const { return presentQueue; }
        uint32_t getPresentQueueFamilyIndex() const { return presentQueueFamilyIndex; }
        VkQueue getComputeQueue() const { return computeQueue; }
        uint32_t getComputeQueueFamilyIndex() const { return computeQueueFamilyIndex; }

        // VkSurfaceKHR and VkSwapchainKHR
        void setSurface(VkSurfaceKHR surf) { surface = surf; }
        VkSurfaceKHR getSurface() const { return surface; }
        const VkSurfaceFormatKHR &getAvailableSurfaceFormats(size_t index) const { return availableSurfaceFormats[index]; }
        uint32_t getAvailableSurfaceFormatCount() const { return static_cast<uint32_t>(availableSurfaceFormats.size()); }
        VkResult getSurfaceFormats();
        VkResult setSurfaceFormat(VkSurfaceFormatKHR format);

        // Swapchain related functions
        VkResult createSwapchain(bool limiteFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);
        VkResult recreateSwapchain();
        VkSwapchainKHR getSwapchain() const { return swapchain; }
        VkImage getSwapchainImage(size_t index) const { return swapchainImages[index]; }
        VkImageView getSwapchainImageView(size_t index) const { return swapchainImageViews[index]; }
        uint32_t getSwapchainImageCount() const { return static_cast<uint32_t>(swapchainImages.size()); }
        const VkSwapchainCreateInfoKHR &getSwapchainCreateInfo() const { return swapchainCreateInfo; }
        uint32_t getCurrentImageIndex() const { return currentImageIndex; }
        VkResult swapImage(VkSemaphore semaphore = VK_NULL_HANDLE);
        VkResult presentImage(VkPresentInfoKHR &presentInfo);
        VkResult presentImage(VkSemaphore waitSemaphore = VK_NULL_HANDLE);

        const std::vector<const char *> &getEnabledLayers() const { return enabledLayers; }
        const std::vector<const char *> &getEnabledExtensions() const { return enabledExtensions; }
        const std::vector<const char *> &getEnabledDeviceExtensions() const { return enabledDeviceExtensions; }

        // Vulkan setup functions
        VkResult checkInstanceLayersSupport(std::span<const char *>) const;
        VkResult checkInstanceExtensionsSupport(std::span<const char *>, const char *layerName = nullptr) const;
        VkResult checkDeviceExtensionsSupport(std::span<const char *>, const char *layerName = nullptr) const;

        // Layer & Extension management
        void addInstanceLayer(const char *layer) { enabledLayers.push_back(layer); }
        void setInstanceLayers(const std::vector<const char *> &layers) { enabledLayers = layers; }
        void addInstanceExtension(const char *extension) { enabledExtensions.push_back(extension); }
        void setInstanceExtensions(const std::vector<const char *> &extensions) { enabledExtensions = extensions; }
        void addDeviceExtension(const char *extension) { enabledDeviceExtensions.push_back(extension); }
        void setDeviceExtensions(const std::vector<const char *> &extensions) { enabledDeviceExtensions = extensions; }

        // callbacks for swapchain recreate
        void registerCreateSwapchainCallback(std::function<void()> func) { onSwapchainCreated.push_back(func); }
        void registerDestroySwapchainCallback(std::function<void()> func) { onSwapchainDestroyed.push_back(func); }
        void registerCreateDeviceCallback(std::function<void()> func) { onDeviceCreated.push_back(func); }
        void registerDestroyDeviceCallback(std::function<void()> func) { onDeviceDestroyed.push_back(func); }

        // Submit command buffer to queues
        VkResult submitCommandBufferToGraphicsQueue(VkSubmitInfo &submitInfo, VkFence fence = VK_NULL_HANDLE);
        VkResult submitCommandBufferToGraphicsQueue(
            VkCommandBuffer commandBuffer,
            VkSemaphore imageAvailable = VK_NULL_HANDLE,
            VkSemaphore renderingIsOver = VK_NULL_HANDLE,
            VkFence fence = VK_NULL_HANDLE,
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        VkResult submitCommandBufferToGraphicsQueue(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE);
        VkResult submitCommandBufferToComputeQueue(VkSubmitInfo &submitInfo, VkFence fence = VK_NULL_HANDLE);
        VkResult submitCommandBufferToComputeQueue(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE);

        // wait for device idle
        VkResult waitIdleDevice() const;

    private:
        // Vulkan Instance
        uint32_t vkApiVersion = VK_API_VERSION_1_0;
        VkInstance instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        std::vector<VkPhysicalDevice> availablePhysicalDevices;

        // Logical Device(graphics, present, compute) & Queues
        VkDevice device;
        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        VkQueue presentQueue;
        uint32_t presentQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        VkQueue computeQueue;
        uint32_t computeQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        // Surface
        VkSurfaceKHR surface;
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;

        // Swapchain
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {}; // cache to recreate swapchain

        // Layer & Extension
        std::vector<const char *> enabledLayers;
        std::vector<const char *> enabledExtensions;
        std::vector<const char *> enabledDeviceExtensions;

        // callbacks
        std::vector<std::function<void()>> onSwapchainCreated;
        std::vector<std::function<void()>> onSwapchainDestroyed;
        std::vector<std::function<void()>> onDeviceCreated;
        std::vector<std::function<void()>> onDeviceDestroyed;

        // current swapchain image index after acquire
        uint32_t currentImageIndex = 0;

        // Debug Messenger
        VkDebugUtilsMessengerEXT debugMessenger;

        // Class functions
        VulkanManager() = default;
        VulkanManager(VulkanManager &&) = delete;
        ~VulkanManager();

        VkResult createSwapchainInternal();
        VkResult checkAndGetQueueFamilyIndices(VkPhysicalDevice physicalDevice);
        void clearImageView();

        // Debug Messenger
        VkResult createDebugMessenger();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        // Execute callbacks
        static void executeCallbacks(const std::vector<std::function<void()>> &callbacks);
    };
}
