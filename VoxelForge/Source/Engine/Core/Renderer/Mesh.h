#pragma once

#include "Vertex.h"
#include "VulkanBuffer.h"

#include <cstdint>
#include <span>

namespace VoxelForge
{
    // Indexed triangle mesh stored in GPU-only memory.
    class Mesh
    {
    public:
        bool Create(
            const VulkanUploadContext& context,
            std::span<const Vertex> vertices,
            std::span<const uint32_t> indices);

        void Destroy();

        // Binds the vertex and index buffers and records the draw. The caller
        // must have bound a compatible pipeline.
        void Draw(VkCommandBuffer commandBuffer) const;

        bool IsValid() const { return vertexBuffer.IsValid() && indexBuffer.IsValid(); }
        uint32_t GetIndexCount() const { return indexCount; }

    private:
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        uint32_t indexCount = 0;
    };
}
