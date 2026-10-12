
#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <glfw3.h>
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
    void RenderFrame();
    void RecreateSwapchain(GLFWwindow* window);

    VkInstance GetInstance() const { return instance; }
    VkSurfaceKHR GetSurface() const { return surface; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetDevice() const { return device; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    VkSwapchainKHR GetSwapchain() const { return swapchain; }

    VkSemaphore GetImageAvailableSemaphore() const
    {
        return imageAvailableSemaphores[currentFrame];
    }

    VkSemaphore GetRenderFinishedSemaphore() const
    {
        return renderFinishedSemaphores[currentFrame];
    }

    VkFence GetInFlightFence() const
    {
        return inFlightFences[currentFrame];
    }

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
    VkRenderPass GetRenderPass() const
    {
        return renderPass;
    }

    const std::vector<VkFramebuffer>& GetFramebuffers() const
    {
        return framebuffers;
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
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateSyncObjects();
    void DrawFrame();

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT]{};

    GLFWwindow* windowHandle = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkCommandBuffer> commandBuffers;

    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent{};
};
