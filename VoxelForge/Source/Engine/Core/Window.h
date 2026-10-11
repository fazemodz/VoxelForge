#pragma once

struct GLFWwindow;

namespace VoxelForge
{
    class Window
    {
    public:
        Window() = default;
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Create(
            const char* title,
            int width,
            int height
        );

        void PollEvents() const;
        void RequestClose() const;

        bool ShouldClose() const;
        GLFWwindow* GetNativeWindow() const;

    private:
        GLFWwindow* m_window = nullptr;
        bool m_glfwInitialized = false;
    };
}