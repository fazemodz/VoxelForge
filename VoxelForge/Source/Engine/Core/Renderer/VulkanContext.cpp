
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include "VulkanContext.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
#ifdef _DEBUG
    constexpr bool EnableValidation = true;
#else
    constexpr bool EnableValidation = false;
#endif

    constexpr const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

    bool IsValidationLayerAvailable()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (const VkLayerProperties& layer : layers)
        {
            if (std::strcmp(layer.layerName, ValidationLayerName) == 0)
                return true;
        }

        return false;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*)
    {
        const char* label =
            (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0
            ? "ERROR" : "WARNING";

        std::cerr << "[Vulkan Validation " << label << "] "
            << callbackData->pMessage << '\n';

        return VK_FALSE;
    }

    VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        return createInfo;
    }
}

VulkanContext::~VulkanContext()
{
    Shutdown();
}

bool VulkanContext::Initialize(GLFWwindow* window)
{
    if (instance != VK_NULL_HANDLE)
    {
        return device != VK_NULL_HANDLE &&
            surface != VK_NULL_HANDLE &&
            swapchain != VK_NULL_HANDLE;
    }

    if (window == nullptr)
    {
        std::cerr << "[Vulkan] Cannot initialize: GLFW window is null.\n";
        return false;
    }

    windowHandle = window;

    std::cout << "[Vulkan] Initializing Vulkan...\n";

    if (!CreateInstance() ||
        !CreateDebugMessenger() ||
        !CreateSurface(window) ||
        !SelectPhysicalDevice() ||
        !CreateLogicalDevice() ||
        !CreateCommandPool() ||
        !CreateCommandBuffers() ||
        !CreateSwapchain(window) ||
        !CreateImageViews() ||
        !CreateDepthResources() ||
        !CreateSwapchainSemaphores() ||
        !CreateRenderPass() ||
        !CreateGraphicsPipeline() ||
        !CreateFramebuffers() ||
        !CreateSyncObjects())
    {
        Shutdown();
        return false;
    }

    std::cout << "[Vulkan] Initialization complete.\n";
    return true;
}

bool VulkanContext::CreateInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VoxelForge";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "VoxelForge Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t extensionCount = 0;
    const char** extensions =
        glfwGetRequiredInstanceExtensions(&extensionCount);

    if (extensions == nullptr || extensionCount == 0)
    {
        std::cerr
            << "[Vulkan] Failed to get required GLFW instance extensions.\n";
        return false;
    }

    std::vector<const char*> enabledExtensions(extensions, extensions + extensionCount);

    validationEnabled = EnableValidation && IsValidationLayerAvailable();

    if (EnableValidation && !validationEnabled)
    {
        std::cout << "[Vulkan] Validation layer not found. Install the Vulkan SDK "
            << "to get API usage errors reported.\n";
    }

    if (validationEnabled)
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    // Chaining the messenger info here also reports problems during
    // vkCreateInstance and vkDestroyInstance.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = MakeDebugMessengerCreateInfo();

    if (validationEnabled)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &ValidationLayerName;
        createInfo.pNext = &debugCreateInfo;
    }

    VkResult result =
        vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create instance. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Instance created successfully"
        << (validationEnabled ? " with validation layers" : "") << ".\n";
    return true;
}

bool VulkanContext::CreateDebugMessenger()
{
    if (!validationEnabled)
        return true;

    auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    const VkDebugUtilsMessengerCreateInfoEXT createInfo = MakeDebugMessengerCreateInfo();

    if (createMessenger == nullptr ||
        createMessenger(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        // Not fatal: the app still runs, it just won't report API misuse.
        std::cerr << "[Vulkan] Failed to create debug messenger.\n";
        debugMessenger = VK_NULL_HANDLE;
    }

    return true;
}

bool VulkanContext::CreateSurface(GLFWwindow* window)
{
    VkResult result = glfwCreateWindowSurface(
        instance,
        window,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create window surface. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Window surface created successfully.\n";
    return true;
}

bool VulkanContext::SelectPhysicalDevice()
{
    uint32_t deviceCount = 0;

    VkResult result = vkEnumeratePhysicalDevices(
        instance,
        &deviceCount,
        nullptr
    );

    if (result != VK_SUCCESS || deviceCount == 0)
    {
        std::cerr << "[Vulkan] No Vulkan-compatible GPUs found.\n";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    result = vkEnumeratePhysicalDevices(
        instance,
        &deviceCount,
        devices.data()
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to enumerate physical devices.\n";
        return false;
    }

    int bestScore = -1;

    std::cout << "\n[Vulkan] Available GPUs:\n";

    for (VkPhysicalDevice candidate : devices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);

        std::cout << "\n  GPU: " << properties.deviceName << '\n';
        std::cout << "  API version: "
            << VK_VERSION_MAJOR(properties.apiVersion) << '.'
            << VK_VERSION_MINOR(properties.apiVersion) << '.'
            << VK_VERSION_PATCH(properties.apiVersion) << '\n';

        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            std::cout << "  Type: Discrete GPU\n";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            std::cout << "  Type: Integrated GPU\n";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            std::cout << "  Type: Virtual GPU\n";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            std::cout << "  Type: CPU\n";
            break;
        default:
            std::cout << "  Type: Other\n";
            break;
        }

        // Check for swapchain extension support.
        uint32_t extensionCount = 0;

        result = vkEnumerateDeviceExtensionProperties(
            candidate,
            nullptr,
            &extensionCount,
            nullptr
        );

        if (result != VK_SUCCESS)
            continue;

        std::vector<VkExtensionProperties> extensions(extensionCount);

        result = vkEnumerateDeviceExtensionProperties(
            candidate,
            nullptr,
            &extensionCount,
            extensions.data()
        );

        if (result != VK_SUCCESS)
            continue;

        bool supportsSwapchain = false;

        for (const auto& extension : extensions)
        {
            if (std::string(extension.extensionName) ==
                VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            {
                supportsSwapchain = true;
                break;
            }
        }

        if (!supportsSwapchain)
        {
            std::cout << "  Swapchain: unsupported\n";
            continue;
        }

        std::cout << "  Swapchain: supported\n";

        // Find a queue family that supports graphics and presentation.
        uint32_t queueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate,
            &queueFamilyCount,
            nullptr
        );

        std::vector<VkQueueFamilyProperties> queueFamilies(
            queueFamilyCount
        );

        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate,
            &queueFamilyCount,
            queueFamilies.data()
        );

        uint32_t suitableQueueFamily = UINT32_MAX;

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (queueFamilies[i].queueCount == 0)
                continue;

            if ((queueFamilies[i].queueFlags &
                VK_QUEUE_GRAPHICS_BIT) == 0)
                continue;

            VkBool32 supportsPresentation = VK_FALSE;

            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                candidate,
                i,
                surface,
                &supportsPresentation
            );

            if (result == VK_SUCCESS &&
                supportsPresentation == VK_TRUE)
            {
                suitableQueueFamily = i;
                break;
            }
        }

        if (suitableQueueFamily == UINT32_MAX)
        {
            std::cout
                << "  Graphics and presentation: unsupported\n";
            continue;
        }

        std::cout
            << "  Graphics and presentation: supported\n";

        int score = 0;

        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = 500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = 300;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score = 100;
            break;
        default:
            score = 200;
            break;
        }

        if (score > bestScore)
        {
            bestScore = score;
            physicalDevice = candidate;
            graphicsQueueFamilyIndex = suitableQueueFamily;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        std::cerr
            << "[Vulkan] No suitable GPU supports graphics, presentation "
            << "and swapchains.\n";
        return false;
    }

    VkPhysicalDeviceProperties selectedProperties{};
    vkGetPhysicalDeviceProperties(
        physicalDevice,
        &selectedProperties
    );

    std::cout << "\n[Vulkan] Selected GPU: "
        << selectedProperties.deviceName << '\n';

    std::cout << "[Vulkan] Graphics/presentation queue family: "
        << graphicsQueueFamilyIndex << '\n';

    return true;
}

bool VulkanContext::CreateLogicalDevice()
{
    if (physicalDevice == VK_NULL_HANDLE ||
        graphicsQueueFamilyIndex == UINT32_MAX)
    {
        std::cerr << "[Vulkan] No suitable physical device selected.\n";
        return false;
    }

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    const char* deviceExtensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VkResult result = vkCreateDevice(
        physicalDevice,
        &createInfo,
        nullptr,
        &device
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create logical device. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Logical device created successfully.\n";

    vkGetDeviceQueue(
        device,
        graphicsQueueFamilyIndex,
        0,
        &graphicsQueue
    );

    if (graphicsQueue == VK_NULL_HANDLE)
    {
        std::cerr << "[Vulkan] Failed to retrieve graphics queue.\n";
        return false;
    }

    std::cout
        << "[Vulkan] Graphics/presentation queue retrieved successfully.\n";

    return true;
}

bool VulkanContext::CreateSwapchain(GLFWwindow* window)
{
    VkSurfaceCapabilitiesKHR capabilities{};

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &capabilities
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to query surface capabilities. Error: "
            << result << '\n';
        return false;
    }

    // Find a suitable surface format.
    uint32_t formatCount = 0;

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        nullptr
    );

    if (result != VK_SUCCESS || formatCount == 0)
    {
        std::cerr << "[Vulkan] No surface formats available.\n";
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        formats.data()
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to retrieve surface formats.\n";
        return false;
    }

    VkSurfaceFormatKHR selectedFormat = formats[0];

    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selectedFormat = format;
            break;
        }
    }

    // Determine the swapchain image dimensions.
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchainExtent = capabilities.currentExtent;
    }
    else
    {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        // A minimized window has no drawable framebuffer yet.
        if (width <= 0 || height <= 0)
        {
            std::cerr
                << "[Vulkan] Window framebuffer has zero size. "
                << "Cannot create swapchain yet.\n";
            return false;
        }

        swapchainExtent.width = std::clamp(
            static_cast<uint32_t>(width),
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );

        swapchainExtent.height = std::clamp(
            static_cast<uint32_t>(height),
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );
    }

    // Request one more image than the minimum, if permitted.
    uint32_t imageCount = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = selectedFormat.format;
    createInfo.imageColorSpace = selectedFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;

    if ((capabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0)
    {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else
    {
        const VkCompositeAlphaFlagBitsKHR options[] =
        {
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };

        bool foundAlphaMode = false;

        for (VkCompositeAlphaFlagBitsKHR option : options)
        {
            if ((capabilities.supportedCompositeAlpha & option) != 0)
            {
                createInfo.compositeAlpha = option;
                foundAlphaMode = true;
                break;
            }
        }

        if (!foundAlphaMode)
        {
            std::cerr << "[Vulkan] No supported composite alpha mode.\n";
            return false;
        }
    }

    // FIFO is guaranteed to be supported by Vulkan.
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(
        device,
        &createInfo,
        nullptr,
        &swapchain
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create swapchain. Error: "
            << result << '\n';
        return false;
    }

    // Retrieve the swapchain images.
    uint32_t actualImageCount = 0;

    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &actualImageCount,
        nullptr
    );

    if (result != VK_SUCCESS || actualImageCount == 0)
    {
        std::cerr << "[Vulkan] Failed to query swapchain images.\n";
        return false;
    }

    swapchainImages.resize(actualImageCount);

    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &actualImageCount,
        swapchainImages.data()
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to retrieve swapchain images.\n";
        swapchainImages.clear();
        return false;
    }

    swapchainImageFormat = selectedFormat.format;

    std::cout << "[Vulkan] Swapchain created successfully.\n";
    std::cout << "[Vulkan] Swapchain size: "
        << swapchainExtent.width << " x "
        << swapchainExtent.height << '\n';
    std::cout << "[Vulkan] Swapchain images: "
        << swapchainImages.size() << '\n';

    return true;
}

bool VulkanContext::CreateImageViews()
{
    if (swapchainImages.empty())
    {
        std::cerr << "[Vulkan] Cannot create image views: no swapchain images.\n";
        return false;
    }

    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(
            device,
            &createInfo,
            nullptr,
            &swapchainImageViews[i]
        );

        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create image view " << i
                << ". Error: " << result << '\n';

            for (VkImageView imageView : swapchainImageViews)
            {
                if (imageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                }
            }

            swapchainImageViews.clear();
            return false;
        }
    }

    std::cout << "[Vulkan] Created "
        << swapchainImageViews.size()
        << " swapchain image views.\n";

    return true;
}

bool VulkanContext::CreateCommandBuffers()
{
    if (commandPool == VK_NULL_HANDLE)
    {
        std::cerr << "[Vulkan] Cannot allocate command buffers: "
            "command pool is missing.\n";
        return false;
    }

    // One per frame in flight, so recording the next frame never touches a
    // command buffer the GPU is still executing.
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount =
        static_cast<uint32_t>(commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(
        device,
        &allocateInfo,
        commandBuffers.data()
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to allocate command buffers. Error: "
            << result << '\n';

        commandBuffers.clear();
        return false;
    }

    std::cout << "[Vulkan] Allocated "
        << commandBuffers.size()
        << " command buffers.\n";

    return true;
}

VkFormat VulkanContext::FindDepthFormat() const
{
    const VkFormat candidates[] =
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        if ((properties.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool VulkanContext::CreateDepthResources()
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE ||
        swapchainImages.empty() || swapchainExtent.width == 0 ||
        swapchainExtent.height == 0)
    {
        std::cerr << "[Vulkan] Cannot create depth resources: renderer or swapchain is not ready.\n";
        return false;
    }

    // Makes this function safe to call again during swapchain recreation.
    DestroyDepthResources();

    depthFormat = FindDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        std::cerr << "[Vulkan] No supported depth attachment format was found.\n";
        return false;
    }

    const size_t count = swapchainImages.size();
    depthImages.assign(count, VK_NULL_HANDLE);
    depthImageMemories.assign(count, VK_NULL_HANDLE);
    depthImageViews.assign(count, VK_NULL_HANDLE);

    for (size_t i = 0; i < count; ++i)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapchainExtent.width;
        imageInfo.extent.height = swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateImage(
            device, &imageInfo, nullptr, &depthImages[i]);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create depth image " << i
                << ". Error: " << result << '\n';
            DestroyDepthResources();
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(device, depthImages[i], &memoryRequirements);

        const uint32_t memoryType = VoxelForge::FindMemoryType(
            physicalDevice,
            memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryType == UINT32_MAX)
        {
            std::cerr << "[Vulkan] No compatible device-local memory for depth image.\n";
            DestroyDepthResources();
            return false;
        }

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = memoryRequirements.size;
        allocationInfo.memoryTypeIndex = memoryType;

        result = vkAllocateMemory(
            device, &allocationInfo, nullptr, &depthImageMemories[i]);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to allocate depth image memory " << i
                << ". Error: " << result << '\n';
            DestroyDepthResources();
            return false;
        }

        result = vkBindImageMemory(
            device, depthImages[i], depthImageMemories[i], 0);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to bind depth image memory " << i
                << ". Error: " << result << '\n';
            DestroyDepthResources();
            return false;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(
            device, &viewInfo, nullptr, &depthImageViews[i]);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create depth image view " << i
                << ". Error: " << result << '\n';
            DestroyDepthResources();
            return false;
        }
    }

    std::cout << "[Vulkan] Created " << depthImageViews.size()
        << " depth images and views.\n";
    return true;
}

void VulkanContext::DestroyDepthResources()
{
    if (device != VK_NULL_HANDLE)
    {
        for (VkImageView view : depthImageViews)
        {
            if (view != VK_NULL_HANDLE)
                vkDestroyImageView(device, view, nullptr);
        }

        for (VkImage image : depthImages)
        {
            if (image != VK_NULL_HANDLE)
                vkDestroyImage(device, image, nullptr);
        }

        for (VkDeviceMemory memory : depthImageMemories)
        {
            if (memory != VK_NULL_HANDLE)
                vkFreeMemory(device, memory, nullptr);
        }
    }

    depthImageViews.clear();
    depthImages.clear();
    depthImageMemories.clear();
    depthFormat = VK_FORMAT_UNDEFINED;
}

bool VulkanContext::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    VkResult result = vkCreateCommandPool(
        device,
        &createInfo,
        nullptr,
        &commandPool
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create command pool. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Command pool created successfully.\n";
    return true;
}
bool VulkanContext::CreateRenderPass()
{
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        std::cerr << "[Vulkan] Cannot create render pass without a depth format.\n";
        return false;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[] =
    {
        colorAttachment,
        depthAttachment
    };

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference{};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(std::size(attachments));
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(
        device, &createInfo, nullptr, &renderPass);

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create render pass. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Render pass with depth attachment created successfully.\n";
    return true;
}

bool VulkanContext::CreateGraphicsPipeline()
{
    VkShaderModule vertexShaderModule =
        VoxelForge::LoadShaderModule(device, "Shaders/triangle.vert.spv");
    VkShaderModule fragmentShaderModule =
        VoxelForge::LoadShaderModule(device, "Shaders/triangle.frag.spv");

    if (vertexShaderModule == VK_NULL_HANDLE || fragmentShaderModule == VK_NULL_HANDLE)
    {
        std::cerr
            << "[Vulkan] Could not load triangle shaders. Check that the "
            << "compiled SPIR-V files are in the Shaders folder relative "
            << "to the application's working directory.\n";

        if (vertexShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(device, vertexShaderModule, nullptr);
        if (fragmentShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

        return false;
    }

    VkPipelineShaderStageCreateInfo vertexStage{};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = vertexShaderModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = fragmentShaderModule;
    fragmentStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertexStage,
        fragmentStage
    };

    using VoxelForge::Vertex;

    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(Vertex);
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{};

    // Position: vec3
    vertexAttributes[0].binding = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[0].offset = offsetof(Vertex, position);

    // Colour: vec3
    vertexAttributes[1].binding = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &vertexBinding;
    vertexInput.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexAttributes.size());
    vertexInput.pVertexAttributeDescriptions = vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor are set per frame in DrawFrame, so the pipeline
    // does not need rebuilding when the window is resized.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    const VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    // Faces are wound counter-clockwise when seen from outside. Together with
    // the Y flip in the projection matrix, that makes them front faces here,
    // so the inside faces of every block can be skipped.
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size =
        static_cast<uint32_t>(sizeof(glm::mat4));

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(
        device,
        &layoutInfo,
        nullptr,
        &pipelineLayout
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create pipeline layout. Error: "
            << result << '\n';

        vkDestroyShaderModule(
            device, fragmentShaderModule, nullptr);

        vkDestroyShaderModule(
            device, vertexShaderModule, nullptr);

        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &graphicsPipeline);

    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create graphics pipeline. Error: "
            << result << '\n';
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
        return false;
    }

    std::cout << "[Vulkan] Graphics pipeline created successfully.\n";
    return true;
}

bool VulkanContext::CreateFramebuffers()
{
    if (renderPass == VK_NULL_HANDLE || swapchainImageViews.empty() ||
        depthImageViews.size() != swapchainImageViews.size())
    {
        std::cerr << "[Vulkan] Cannot create framebuffers: render pass, color views, "
            << "or matching depth views are missing.\n";
        return false;
    }

    framebuffers.resize(swapchainImageViews.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] =
        {
            swapchainImageViews[i],
            depthImageViews[i]
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(std::size(attachments));
        createInfo.pAttachments = attachments;
        createInfo.width = swapchainExtent.width;
        createInfo.height = swapchainExtent.height;
        createInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(
            device, &createInfo, nullptr, &framebuffers[i]);

        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create framebuffer " << i
                << ". Error: " << result << '\n';

            for (VkFramebuffer framebuffer : framebuffers)
            {
                if (framebuffer != VK_NULL_HANDLE)
                    vkDestroyFramebuffer(device, framebuffer, nullptr);
            }

            framebuffers.clear();
            return false;
        }
    }

    std::cout << "[Vulkan] Created " << framebuffers.size()
        << " framebuffers with depth attachments.\n";
    return true;
}

bool VulkanContext::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(
            device, &semaphoreInfo, nullptr,
            &imageAvailableSemaphores[i]) != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create image-available semaphore.\n";
            return false;
        }

        if (vkCreateFence(
            device, &fenceInfo, nullptr,
            &inFlightFences[i]) != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create in-flight fence.\n";
            return false;
        }
    }

    std::cout << "[Vulkan] Frame synchronisation created successfully.\n";
    return true;
}

bool VulkanContext::CreateSwapchainSemaphores()
{
    DestroySwapchainSemaphores();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    renderFinishedSemaphores.assign(swapchainImages.size(), VK_NULL_HANDLE);

    for (VkSemaphore& semaphore : renderFinishedSemaphores)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create render-finished semaphore.\n";
            DestroySwapchainSemaphores();
            return false;
        }
    }

    return true;
}

void VulkanContext::DestroySwapchainSemaphores()
{
    if (device != VK_NULL_HANDLE)
    {
        for (VkSemaphore semaphore : renderFinishedSemaphores)
        {
            if (semaphore != VK_NULL_HANDLE)
                vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    renderFinishedSemaphores.clear();
}

void VulkanContext::DrawFrame(const VoxelForge::Camera& camera)
{
    if (windowHandle == nullptr)
        return;

    // A minimized window has no drawable framebuffer. Wait for events rather
    // than trying to create a zero-sized swapchain.
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(windowHandle, &framebufferWidth, &framebufferHeight);

    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;

    VkFence inFlightFence = inFlightFences[currentFrame];

    if (vkWaitForFences(
        device, 1, &inFlightFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed waiting for frame fence.\n";
        return;
    }

    uint32_t imageIndex = 0;

    VkResult result = vkAcquireNextImageKHR(
        device,
        swapchain,
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapchain(windowHandle);
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "[Vulkan] Failed to acquire swapchain image. Error: "
            << result << '\n';
        return;
    }

    const bool acquireSuboptimal = (result == VK_SUBOPTIMAL_KHR);

    if (imageIndex >= framebuffers.size() ||
        imageIndex >= renderFinishedSemaphores.size())
    {
        std::cerr << "[Vulkan] Acquired image index is out of range.\n";
        return;
    }

    VkCommandBuffer commandBuffer = commandBuffers[currentFrame];

    if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to reset command buffer.\n";
        return;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to begin command buffer.\n";
        return;
    }

    VkClearValue clearValues[2]{};
    clearValues[0].color = {{ 0.08f, 0.12f, 0.20f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE
    );

    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline
    );

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    const glm::mat4 viewProjection =
        camera.GetProjectionMatrix(GetAspectRatio()) * camera.GetViewMatrix();

    for (const DrawCommand& command : drawQueue)
    {
        const glm::mat4 mvp = viewProjection * command.model;

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            static_cast<uint32_t>(sizeof(glm::mat4)),
            glm::value_ptr(mvp)
        );

        command.mesh->Draw(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to record command buffer.\n";
        return;
    }

    VkSemaphore waitSemaphores[] = {
        imageAvailableSemaphores[currentFrame]
    };

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore signalSemaphores[] = {
        renderFinishedSemaphores[imageIndex]
    };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset the fence only when we are ready to submit work that will signal it.
    if (vkResetFences(device, 1, &inFlightFence) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to reset frame fence.\n";
        return;
    }

    if (vkQueueSubmit(
        graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to submit draw command.\n";
        return;
    }

    VkSwapchainKHR swapchains[] = { swapchain };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

    const bool needsRecreation =
        acquireSuboptimal ||
        result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR;

    if (result != VK_SUCCESS &&
        result != VK_ERROR_OUT_OF_DATE_KHR &&
        result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "[Vulkan] Failed to present frame. Error: "
            << result << '\n';
        return;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    if (needsRecreation)
        RecreateSwapchain(windowHandle);
}

void VulkanContext::Submit(const VoxelForge::Mesh& mesh, const glm::mat4& model)
{
    if (mesh.IsValid())
        drawQueue.push_back({ &mesh, model });
}

void VulkanContext::RenderFrame(const VoxelForge::Camera& camera)
{
    if (device != VK_NULL_HANDLE &&
        swapchain != VK_NULL_HANDLE &&
        !commandBuffers.empty() &&
        windowHandle != nullptr)
    {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(windowHandle, &framebufferWidth, &framebufferHeight);

        // Skip rendering while minimized. The next call will try again after restore.
        const bool minimized = framebufferWidth <= 0 || framebufferHeight <= 0;

        if (!minimized &&
            (static_cast<uint32_t>(framebufferWidth) != swapchainExtent.width ||
             static_cast<uint32_t>(framebufferHeight) != swapchainExtent.height))
        {
            RecreateSwapchain(windowHandle);
        }
        else if (!minimized)
        {
            DrawFrame(camera);
        }
    }

    // Submissions are per frame, even when nothing was drawn.
    drawQueue.clear();
}

void VulkanContext::RecreateSwapchain(GLFWwindow* window)
{
    if (device == VK_NULL_HANDLE || window == nullptr)
        return;

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    // When minimized, wait until the window has a non-zero framebuffer.
    while ((width <= 0 || height <= 0) &&
        glfwWindowShouldClose(window) == GLFW_FALSE)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    if (width <= 0 || height <= 0)
        return;

    if (vkDeviceWaitIdle(device) != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to wait for device before swapchain recreation.\n";
        return;
    }

    std::cout << "[Vulkan] Recreating swapchain...\n";

    // The render pass and pipeline only depend on the image formats, and the
    // viewport is dynamic, so only size-dependent objects are rebuilt.
    DestroySwapchainResources();

    if (!CreateSwapchain(window) ||
        !CreateImageViews() ||
        !CreateDepthResources() ||
        !CreateSwapchainSemaphores() ||
        !CreateFramebuffers())
    {
        std::cerr << "[Vulkan] Swapchain recreation failed.\n";
        return;
    }

    std::cout << "[Vulkan] Swapchain recreation complete.\n";
}

void VulkanContext::DestroySwapchainResources()
{
    if (device == VK_NULL_HANDLE)
        return;

    for (VkFramebuffer framebuffer : framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    framebuffers.clear();

    DestroySwapchainSemaphores();
    DestroyDepthResources();

    for (VkImageView imageView : swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, imageView, nullptr);
    }
    swapchainImageViews.clear();

    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    swapchainImages.clear();
    swapchainImageFormat = VK_FORMAT_UNDEFINED;
    swapchainExtent = {};
}

void VulkanContext::Shutdown()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device);

        DestroySwapchainResources();

        if (graphicsPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }

        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        if (renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }

            if (inFlightFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(device, inFlightFences[i], nullptr);
                inFlightFences[i] = VK_NULL_HANDLE;
            }
        }

        currentFrame = 0;
        drawQueue.clear();

        // Destroying the pool also frees the command buffers allocated from it.
        commandBuffers.clear();
        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    graphicsQueue = VK_NULL_HANDLE;

    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    if (debugMessenger != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

        if (destroyMessenger != nullptr)
            destroyMessenger(instance, debugMessenger, nullptr);

        debugMessenger = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    validationEnabled = false;
    physicalDevice = VK_NULL_HANDLE;
    graphicsQueueFamilyIndex = UINT32_MAX;
    windowHandle = nullptr;
}
