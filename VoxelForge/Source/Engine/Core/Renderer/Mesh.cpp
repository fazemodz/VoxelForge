#include "Mesh.h"

#include <iostream>

namespace VoxelForge
{
    bool Mesh::Create(
        const VulkanUploadContext& context,
        std::span<const Vertex> vertices,
        std::span<const uint32_t> indices)
    {
        Destroy();

        if (vertices.empty() || indices.empty())
        {
            std::cerr << "[Vulkan] Cannot create mesh: no vertices or indices.\n";
            return false;
        }

        if (!vertexBuffer.CreateDeviceLocal(
            context,
            vertices.data(),
            vertices.size_bytes(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
            !indexBuffer.CreateDeviceLocal(
                context,
                indices.data(),
                indices.size_bytes(),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            Destroy();
            return false;
        }

        indexCount = static_cast<uint32_t>(indices.size());
        return true;
    }

    void Mesh::Destroy()
    {
        indexBuffer.Destroy();
        vertexBuffer.Destroy();
        indexCount = 0;
    }

    void Mesh::Draw(VkCommandBuffer commandBuffer) const
    {
        if (!IsValid())
            return;

        const VkBuffer vertexBuffers[] = { vertexBuffer.GetHandle() };
        const VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    }
}
