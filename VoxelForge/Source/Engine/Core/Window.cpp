
#include "Window.h"

#include <glfw3.h>

#include <iostream>

namespace VoxelForge
{
    Window::~Window()
    {
        if (m_window != nullptr)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }

        if (m_glfwInitialized)
        {
            glfwTerminate();
            m_glfwInitialized = false;
        }
    }

    bool Window::Create(const char* title, int width, int height)
    {
        if (m_window != nullptr)
        {
            std::cerr << "[Window] Window already exists.\n";
            return false;
        }

        if (glfwInit() != GLFW_TRUE)
        {
            std::cerr << "[Window] Failed to initialize GLFW.\n";
            return false;
        }

        m_glfwInitialized = true;

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

        glfwSetWindowUserPointer(m_window, this);

        glfwSetFramebufferSizeCallback(
            m_window,
            FramebufferResizeCallback
        );

        m_framebufferResized = false;

        std::cout << "[Window] Created successfully.\n";

        return true;
    }

    void Window::PollEvents() const
    {
        glfwPollEvents();
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
        if (m_window == nullptr)
        {
            return true;
        }

        return glfwWindowShouldClose(m_window) == GLFW_TRUE;
    }

    GLFWwindow* Window::GetNativeWindow() const
    {
        return m_window;
    }

    void Window::FramebufferResizeCallback(
        GLFWwindow* window,
        int width,
        int height)
    {
        (void)width;
        (void)height;

        auto* appWindow = static_cast<Window*>(
            glfwGetWindowUserPointer(window)
            );

        if (appWindow != nullptr)
        {
            appWindow->m_framebufferResized = true;
        }
    }
}
