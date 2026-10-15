#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace VoxelForge
{
    // Everything needed to record and submit a one-off transfer to the GPU.
    struct VulkanUploadContext
    {
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
    };

    // Owns a VkBuffer and the memory bound to it. Move-only; the buffer is
    // destroyed automatically when the object goes out of scope.
    class VulkanBuffer
    {
    public:
        VulkanBuffer() = default;
        ~VulkanBuffer();

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        bool Create(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties);

        // Creates a GPU-only buffer and fills it through a temporary staging
        // buffer. Blocks until the copy has finished.
        bool CreateDeviceLocal(
            const VulkanUploadContext& context,
            const void* data,
            VkDeviceSize size,
            VkBufferUsageFlags usage);

        // Copies data into a host-visible buffer.
        bool Write(const void* data, VkDeviceSize dataSize, VkDeviceSize offset = 0);

        void Destroy();

        VkBuffer GetHandle() const { return buffer; }
        VkDeviceSize GetSize() const { return size; }
        bool IsValid() const { return buffer != VK_NULL_HANDLE; }

    private:
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    };

    // Returns UINT32_MAX when no memory type matches.
    uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties);
}
