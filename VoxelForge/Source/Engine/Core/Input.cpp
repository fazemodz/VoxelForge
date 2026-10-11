#include "Input.h"

#include <glfw3.h>

namespace VoxelForge
{
    bool Input::IsKeyDown(GLFWwindow* window, int key)
    {
        if (window == nullptr)
        {
            return false;
        }

        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    bool Input::IsMouseButtonDown(
        GLFWwindow* window,
        int button)
    {
        if (window == nullptr)
        {
            return false;
        }

        return glfwGetMouseButton(window, button) == GLFW_PRESS;
    }

    void Input::GetMousePosition(
        GLFWwindow* window,
        double& x,
        double& y)
    {
        x = 0.0;
        y = 0.0;

        if (window != nullptr)
        {
            glfwGetCursorPos(window, &x, &y);
        }
    }
}