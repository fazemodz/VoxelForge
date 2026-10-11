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

        if (!m_window.Create(
            "VoxelForge",
            1280,
            720))
        {
            std::cerr << "Failed to initialize VoxelForge.\n";
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
            // Process keyboard, mouse and window events.
            m_window.PollEvents();

            // Update frame timing.
            Time::Tick();

            GLFWwindow* nativeWindow =
                m_window.GetNativeWindow();

            // Escape requests a clean shutdown.
            if (Input::IsKeyDown(
                nativeWindow,
                GLFW_KEY_ESCAPE))
            {
                m_window.RequestClose();
            }

            // Game and engine updates will go here later in dev
          
        }

        std::cout << "Leaving main loop.\n";

        m_initialized = false;

        return 0;
    }
}