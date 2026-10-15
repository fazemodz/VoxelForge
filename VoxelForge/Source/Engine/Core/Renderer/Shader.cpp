#include "Shader.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace VoxelForge
{
    namespace
    {
        bool ReadShaderFile(const char* path, std::vector<char>& code)
        {
            std::ifstream file(path, std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                std::cerr << "[Vulkan] Failed to open shader file: " << path << '\n';
                return false;
            }

            const std::streamsize fileSize = file.tellg();
            if (fileSize <= 0 || fileSize % 4 != 0)
            {
                std::cerr << "[Vulkan] Shader file has an invalid size: " << path << '\n';
                return false;
            }

            code.resize(static_cast<size_t>(fileSize));
            file.seekg(0);
            file.read(code.data(), fileSize);

            if (!file)
            {
                std::cerr << "[Vulkan] Failed to read shader file: " << path << '\n';
                code.clear();
                return false;
            }

            return true;
        }
    }

    VkShaderModule LoadShaderModule(VkDevice device, const char* path)
    {
        std::vector<char> code;
        if (!ReadShaderFile(path, code))
            return VK_NULL_HANDLE;

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        const VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

        if (result != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Failed to create shader module from " << path
                << ". Error: " << result << '\n';
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }
}
