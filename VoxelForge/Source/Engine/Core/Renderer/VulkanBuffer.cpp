#include "VulkanBuffer.h"

#include <cstring>
#include <iostream>
#include <utility>

namespace VoxelForge
{
    uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            const bool typeMatches = (typeFilter & (1u << i)) != 0;
            const bool propertiesMatch =
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

            if (typeMatches && propertiesMatch)
                return i;
        }

        return UINT32_MAX;
    }

    VulkanBuffer::~VulkanBuffer()
    {
        Destroy();
    }

    VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
        : device(std::exchange(other.device, VK_NULL_HANDLE)),
          buffer(std::exchange(other.buffer, VK_NULL_HANDLE)),
          memory(std::exchange(other.memory, VK_NULL_HANDLE)),
          size(std::exchange(other.size, 0))
    {
    }

    VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            device = std::exchange(other.device, VK_NULL_HANDLE);
            buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
            memory = std::exchange(other.memory, VK_NULL_HANDLE);
            size = std::exchange(other.size, 0);
        }

        return *this;
    }

    bool VulkanBuffer::Create(
        VkDevice newDevice,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize newSize,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties)
    {
        Destroy();

        if (newDevice == VK_NULL_HANDLE || newSize == 0)
        {
            std::cerr << "[Vulkan] Cannot create buffer: no device or zero size.\n";
            return false;
        }

        device = newDevice;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = newSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create GPU buffer. Error: "
                << result << '\n';
            Destroy();
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        const uint32_t memoryType = FindMemoryType(
            physicalDevice, memoryRequirements.memoryTypeBits, properties);

        if (memoryType == UINT32_MAX)
        {
            std::cerr << "[Vulkan] No compatible memory type found for buffer.\n";
            Destroy();
            return false;
        }

        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.allocationSize = memoryRequirements.size;
        allocationInfo.memoryTypeIndex = memoryType;

        result = vkAllocateMemory(device, &allocationInfo, nullptr, &memory);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to allocate buffer memory. Error: "
                << result << '\n';
            Destroy();
            return false;
        }

        result = vkBindBufferMemory(device, buffer, memory, 0);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to bind buffer memory. Error: "
                << result << '\n';
            Destroy();
            return false;
        }

        size = newSize;
        return true;
    }

    bool VulkanBuffer::CreateDeviceLocal(
        const VulkanUploadContext& context,
        const void* data,
        VkDeviceSize dataSize,
        VkBufferUsageFlags usage)
    {
        if (data == nullptr || context.commandPool == VK_NULL_HANDLE ||
            context.queue == VK_NULL_HANDLE)
        {
            std::cerr << "[Vulkan] Cannot upload buffer: missing data, command pool or queue.\n";
            return false;
        }

        VulkanBuffer staging;
        if (!staging.Create(
            context.device,
            context.physicalDevice,
            dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
            !staging.Write(data, dataSize))
        {
            return false;
        }

        if (!Create(
            context.device,
            context.physicalDevice,
            dataSize,
            usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            return false;
        }

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = context.commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(context.device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to allocate upload command buffer.\n";
            Destroy();
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkBufferCopy copyRegion{};
        copyRegion.size = dataSize;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        bool succeeded = vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS;

        if (succeeded)
        {
            vkCmdCopyBuffer(commandBuffer, staging.GetHandle(), buffer, 1, &copyRegion);
            succeeded = vkEndCommandBuffer(commandBuffer) == VK_SUCCESS;
        }

        // Waiting for the queue keeps this simple. Batch uploads once chunk
        // meshes start streaming in.
        if (succeeded)
        {
            succeeded =
                vkQueueSubmit(context.queue, 1, &submitInfo, VK_NULL_HANDLE) == VK_SUCCESS &&
                vkQueueWaitIdle(context.queue) == VK_SUCCESS;
        }

        vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);

        if (!succeeded)
        {
            std::cerr << "[Vulkan] Failed to copy staging buffer to the GPU.\n";
            Destroy();
            return false;
        }

        return true;
    }

    bool VulkanBuffer::Write(const void* data, VkDeviceSize dataSize, VkDeviceSize offset)
    {
        if (memory == VK_NULL_HANDLE || offset + dataSize > size)
        {
            std::cerr << "[Vulkan] Buffer write is out of range.\n";
            return false;
        }

        void* mappedMemory = nullptr;
        const VkResult result = vkMapMemory(device, memory, offset, dataSize, 0, &mappedMemory);
        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to map buffer memory. Error: "
                << result << '\n';
            return false;
        }

        std::memcpy(mappedMemory, data, static_cast<size_t>(dataSize));
        vkUnmapMemory(device, memory);
        return true;
    }

    void VulkanBuffer::Destroy()
    {
        if (device != VK_NULL_HANDLE)
        {
            if (buffer != VK_NULL_HANDLE)
                vkDestroyBuffer(device, buffer, nullptr);

            if (memory != VK_NULL_HANDLE)
                vkFreeMemory(device, memory, nullptr);
        }

        device = VK_NULL_HANDLE;
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        size = 0;
    }
}
