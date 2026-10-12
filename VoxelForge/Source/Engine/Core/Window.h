
#pragma once

#include <glfw3.h>

namespace VoxelForge
{
    class Window
    {
    public:
        Window() = default;
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Create(const char* title, int width, int height);

        void PollEvents() const;
        void RequestClose() const;

        bool ShouldClose() const;

        GLFWwindow* GetNativeWindow() const;

        static void FramebufferResizeCallback(
            GLFWwindow* window,
            int width,
            int height
        );

        bool WasResized() const
        {
            return m_framebufferResized;
        }

        void ResetResizedFlag()
        {
            m_framebufferResized = false;
        }

    private:
        GLFWwindow* m_window = nullptr;
        bool m_glfwInitialized = false;
        bool m_framebufferResized = false;
    };
}
