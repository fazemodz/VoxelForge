#pragma once

#include "Window.h"
#include "Renderer/VulkanContext.h"

namespace VoxelForge
{
    class Application
    {
    public:
        bool Initialize();
        int Run();

    private:
        Window m_window;
        VulkanContext m_vulkanContext;
        bool m_initialized = false;
    };
}