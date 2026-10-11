#pragma once

#include "Window.h"

namespace VoxelForge
{
    class Application
    {
    public:
        Application() = default;
        ~Application() = default;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        bool Initialize();
        int Run();

    private:
        Window m_window;
        bool m_initialized = false;
    };
}