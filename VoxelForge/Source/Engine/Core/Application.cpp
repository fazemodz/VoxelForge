#include "Application.h"

#include "Input.h"
#include "Time.h"

#include <glfw3.h>
#include <iostream>

namespace VoxelForge
{
    bool Application::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        std::cout << "Starting VoxelForge...\n";

        if (!m_window.Create("VoxelForge", 1280, 720))
        {
            std::cerr << "Failed to initialize VoxelForge window.\n";
            return false;
        }

        // Initialise Vulkan after GLFW has created the window.
        if (!m_vulkanContext.Initialize(m_window.GetNativeWindow()))
        {
            std::cerr << "Failed to initialize Vulkan.\n";
            m_vulkanContext.Shutdown();
            return false;
        }

        Time::Reset();

        m_initialized = true;
        std::cout << "VoxelForge initialized.\n";

        return true;
    }

    int Application::Run()
    {
        if (!m_initialized)
        {
            std::cerr << "Cannot run: application is not initialized.\n";
            return 1;
        }

        std::cout << "Entering main loop.\n";

        while (!m_window.ShouldClose())
        {
            m_window.PollEvents();
            Time::Tick();

            GLFWwindow* nativeWindow = m_window.GetNativeWindow();

            if (Input::IsKeyDown(nativeWindow, GLFW_KEY_ESCAPE))
            {
                m_window.RequestClose();
            }

            // Rendering and engine updates will be added later.
            m_vulkanContext.RenderFrame();
        }

        std::cout << "Leaving main loop.\n";

        // Destroy Vulkan resources before shutting down the window.
        m_vulkanContext.Shutdown();

        m_initialized = false;
        return 0;
    }
}