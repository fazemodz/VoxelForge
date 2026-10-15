#pragma once

#include <vulkan/vulkan.h>

namespace VoxelForge
{
    // Loads a compiled SPIR-V file and wraps it in a shader module.
    // Returns VK_NULL_HANDLE on failure. The caller owns the module.
    VkShaderModule LoadShaderModule(VkDevice device, const char* path);
}
