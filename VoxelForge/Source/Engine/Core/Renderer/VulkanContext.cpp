
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

    windowHandle = window;

    std::cout << "[Vulkan] Initializing Vulkan...\n";

    if (!CreateInstance() ||
        !CreateSurface(window) ||
        !SelectPhysicalDevice() ||
        !CreateLogicalDevice() ||
        !CreateSwapchain(window) ||
        !CreateImageViews() ||
        !CreateCommandPool() ||
        !CreateCommandBuffers() ||
        !CreateRenderPass() ||
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
bool VulkanContext::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(
        device,
        &createInfo,
        nullptr,
        &renderPass
    );

    if (result != VK_SUCCESS)
    {
        std::cerr << "[Vulkan] Failed to create render pass. Error: "
            << result << '\n';
        return false;
    }

    std::cout << "[Vulkan] Render pass created successfully.\n";
    return true;
}
bool VulkanContext::CreateFramebuffers()
{
    if (renderPass == VK_NULL_HANDLE || swapchainImageViews.empty())
    {
        std::cerr << "[Vulkan] Cannot create framebuffers: "
            "render pass or image views are missing.\n";
        return false;
    }

    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {
            swapchainImageViews[i]
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = swapchainExtent.width;
        createInfo.height = swapchainExtent.height;
        createInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(
            device,
            &createInfo,
            nullptr,
            &framebuffers[i]
        );

        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create framebuffer "
                << i << ". Error: " << result << '\n';

            for (VkFramebuffer framebuffer : framebuffers)
            {
                if (framebuffer != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(
                        device, framebuffer, nullptr
                    );
                }
            }

            framebuffers.clear();
            return false;
        }
    }

    std::cout << "[Vulkan] Created "
        << framebuffers.size()
        << " framebuffers.\n";

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

        if (vkCreateSemaphore(
            device, &semaphoreInfo, nullptr,
            &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create render-finished semaphore.\n";
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
void VulkanContext::DrawFrame()
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

    if (imageIndex >= commandBuffers.size() ||
        imageIndex >= framebuffers.size())
    {
        std::cerr << "[Vulkan] Acquired image index is out of range.\n";
        return;
    }

    VkCommandBuffer commandBuffer = commandBuffers[imageIndex];

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

    VkClearValue clearValue{};
    clearValue.color = { { 0.08f, 0.12f, 0.20f, 1.0f } };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE
    );

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
        renderFinishedSemaphores[currentFrame]
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

void VulkanContext::RenderFrame()
{
    if (device == VK_NULL_HANDLE ||
        swapchain == VK_NULL_HANDLE ||
        commandBuffers.empty() ||
        windowHandle == nullptr)
    {
        return;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(windowHandle, &framebufferWidth, &framebufferHeight);

    // Skip rendering while minimized. The next call will try again after restore.
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;

    if (static_cast<uint32_t>(framebufferWidth) != swapchainExtent.width ||
        static_cast<uint32_t>(framebufferHeight) != swapchainExtent.height)
    {
        RecreateSwapchain(windowHandle);
        return;
    }

    DrawFrame();
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

    // Destroy objects that depend on the old swapchain, in dependency order.
    for (VkFramebuffer framebuffer : framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    framebuffers.clear();

    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    if (!commandBuffers.empty() && commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(
            device,
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
        commandBuffers.clear();
    }

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

    if (!CreateSwapchain(window) ||
        !CreateImageViews() ||
        !CreateCommandBuffers() ||
        !CreateRenderPass() ||
        !CreateFramebuffers())
    {
        std::cerr << "[Vulkan] Swapchain recreation failed.\n";
        return;
    }

    std::cout << "[Vulkan] Swapchain recreation complete.\n";
}

void VulkanContext::Shutdown()
{
    if (device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device);

    for (VkFramebuffer framebuffer : framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }

    framebuffers.clear();

    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

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
        commandBuffers.clear();

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(
                    device, imageAvailableSemaphores[i], nullptr);
                imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }

            if (renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(
                    device, renderFinishedSemaphores[i], nullptr);
                renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }

            if (inFlightFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(device, inFlightFences[i], nullptr);
                inFlightFences[i] = VK_NULL_HANDLE;
            }
        }

        currentFrame = 0;
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
    windowHandle = nullptr;
}
