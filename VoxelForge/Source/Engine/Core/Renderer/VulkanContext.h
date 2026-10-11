
#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

struct GLFWwindow;

class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    bool Initialize(GLFWwindow* window);
    void Shutdown();

    VkInstance GetInstance() const { return instance; }
    VkSurfaceKHR GetSurface() const { return surface; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetDevice() const { return device; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    VkSwapchainKHR GetSwapchain() const { return swapchain; }

    uint32_t GetGraphicsQueueFamilyIndex() const
    {
        return graphicsQueueFamilyIndex;
    }

    const std::vector<VkImage>& GetSwapchainImages() const
    {
        return swapchainImages;
    }

    VkFormat GetSwapchainImageFormat() const
    {
        return swapchainImageFormat;
    }

    VkExtent2D GetSwapchainExtent() const
    {
        return swapchainExtent;
    }
    const std::vector<VkImageView>& GetSwapchainImageViews() const
    {
        return swapchainImageViews;
    }
    VkCommandPool GetCommandPool() const
    {
        return commandPool;
    }
    const std::vector<VkCommandBuffer>& GetCommandBuffers() const
    {
        return commandBuffers;
    }
private:
    bool CreateInstance();
    bool CreateSurface(GLFWwindow* window);
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain(GLFWwindow* window);
    bool CreateImageViews();
    bool CreateCommandPool();
    bool CreateCommandBuffers();

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkCommandBuffer> commandBuffers;

    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent{};
};
