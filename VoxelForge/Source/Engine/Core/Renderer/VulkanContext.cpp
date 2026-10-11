
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include "VulkanContext.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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

    std::cout << "[Vulkan] Initializing Vulkan...\n";

    if (!CreateInstance() ||
        !CreateSurface(window) ||
        !SelectPhysicalDevice() ||
        !CreateLogicalDevice() ||
        !CreateSwapchain(window) ||
        !CreateImageViews() ||
        !CreateCommandPool() ||
        !CreateCommandBuffers())
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

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkResult result =
        vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create instance. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Instance created successfully.\n";
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
    if (commandPool == VK_NULL_HANDLE || swapchainImages.empty())
    {
        std::cerr << "[Vulkan] Cannot allocate command buffers: "
            "command pool or swapchain images are missing.\n";
        return false;
    }

    commandBuffers.resize(swapchainImages.size());

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
void VulkanContext::Shutdown()
{
    if (device != VK_NULL_HANDLE)
    {
        for (VkImageView imageView : swapchainImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }

        swapchainImageViews.clear();
        vkDeviceWaitIdle(device);
        commandBuffers.clear(); commandBuffers.clear();

        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }

        if (swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }

        swapchainImages.clear();
        swapchainImageFormat = VK_FORMAT_UNDEFINED;
        swapchainExtent = {};

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    graphicsQueue = VK_NULL_HANDLE;

    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    physicalDevice = VK_NULL_HANDLE;
    graphicsQueueFamilyIndex = UINT32_MAX;
}
