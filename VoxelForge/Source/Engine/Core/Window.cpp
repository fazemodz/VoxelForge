#include "Window.h"

#include <glfw3.h>

#include <iostream>

namespace VoxelForge
{
    namespace
    {
        void GLFWErrorCallback(int error, const char* description)
        {
            std::cerr
                << "[GLFW] Error " << error
                << ": " << description
                << '\n';
        }
    }

    bool Window::Create(
        const char* title,
        int width,
        int height)
    {
        if (m_window != nullptr || m_glfwInitialized)
        {
            std::cerr << "[Window] Window already initialized.\n";
            return false;
        }

        if (width <= 0 || height <= 0)
        {
            std::cerr << "[Window] Invalid window dimensions.\n";
            return false;
        }

        glfwSetErrorCallback(GLFWErrorCallback);

        if (glfwInit() != GLFW_TRUE)
        {
            std::cerr << "[Window] Failed to initialize GLFW.\n";
            return false;
        }

        m_glfwInitialized = true;

        // Do not create an OpenGL or OpenGL ES context.
        // Vulkan will provide the graphics API.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(
            width,
            height,
            title,
            nullptr,
            nullptr
        );

        if (m_window == nullptr)
        {
            std::cerr << "[Window] Failed to create window.\n";

            glfwTerminate();
            m_glfwInitialized = false;

            return false;
        }

        std::cout << "[Window] Created successfully.\n";

        return true;
    }

    void Window::PollEvents() const
    {
        if (m_window != nullptr)
        {
            glfwPollEvents();
        }
    }

    void Window::RequestClose() const
    {
        if (m_window != nullptr)
        {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
    }

    bool Window::ShouldClose() const
    {
        return m_window == nullptr ||
            glfwWindowShouldClose(m_window) == GLFW_TRUE;
    }

    GLFWwindow* Window::GetNativeWindow() const
    {
        return m_window;
    }

    Window::~Window()
    {
        if (m_window != nullptr)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;

            std::cout << "[Window] Destroyed.\n";
        }

        if (m_glfwInitialized)
        {
            glfwTerminate();
            m_glfwInitialized = false;

            std::cout << "[Window] GLFW terminated.\n";
        }
    }
}