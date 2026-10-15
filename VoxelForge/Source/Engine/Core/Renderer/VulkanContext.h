
#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <glfw3.h>

#include "Camera.h"
#include "Mesh.h"

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
    void RecreateSwapchain(GLFWwindow* window);

    // Queues a mesh to be drawn with the given model transform in the next
    // RenderFrame. The mesh must stay alive until RenderFrame returns.
    void Submit(const VoxelForge::Mesh& mesh, const glm::mat4& model);

    // Draws everything submitted since the last call, then clears the queue.
    void RenderFrame(const VoxelForge::Camera& camera);

    // Blocks until the GPU has finished all submitted frames. Call before
    // destroying meshes or other resources that recent frames may still use.
    void WaitIdle() const
    {
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);
    }

    // Pass this to Mesh::Create to upload geometry to the GPU.
    VoxelForge::VulkanUploadContext GetUploadContext() const
    {
        return { device, physicalDevice, commandPool, graphicsQueue };
    }

    float GetAspectRatio() const
    {
        return swapchainExtent.height == 0
            ? 1.0f
            : static_cast<float>(swapchainExtent.width) /
              static_cast<float>(swapchainExtent.height);
    }

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
    bool CreateDebugMessenger();
    bool CreateSurface(GLFWwindow* window);
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain(GLFWwindow* window);
    bool CreateImageViews();
    bool CreateDepthResources();
    VkFormat FindDepthFormat() const;
    void DestroyDepthResources();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateRenderPass();
    bool CreateGraphicsPipeline();
    bool CreateFramebuffers();
    bool CreateSyncObjects();
    bool CreateSwapchainSemaphores();
    void DestroySwapchainSemaphores();
    void DestroySwapchainResources();
    void DrawFrame(const VoxelForge::Camera& camera);

    struct DrawCommand
    {
        const VoxelForge::Mesh* mesh = nullptr;
        glm::mat4 model{ 1.0f };
    };

    std::vector<DrawCommand> drawQueue;

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // Per frame in flight: the CPU records frame N+1 while the GPU draws frame N.
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT]{};
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT]{};

    // Per swapchain image: presentation of an image may still be waiting on
    // its semaphore when a later frame starts, so it cannot be shared per frame.
    std::vector<VkSemaphore> renderFinishedSemaphores;

    GLFWwindow* windowHandle = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool validationEnabled = false;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemories;
    std::vector<VkImageView> depthImageViews;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkCommandBuffer> commandBuffers;

    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent{};
};
