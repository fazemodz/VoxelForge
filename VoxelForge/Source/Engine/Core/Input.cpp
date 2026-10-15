#include "Input.h"

#include <glfw3.h>

#include <bitset>

namespace VoxelForge
{
    namespace
    {
        constexpr int KeyCount = GLFW_KEY_LAST + 1;
        constexpr int MouseButtonCount = GLFW_MOUSE_BUTTON_LAST + 1;

        std::bitset<KeyCount> s_keysDown;
        std::bitset<KeyCount> s_keysPressed;
        std::bitset<MouseButtonCount> s_buttonsDown;
        std::bitset<MouseButtonCount> s_buttonsPressed;

        double s_lastMouseX = 0.0;
        double s_lastMouseY = 0.0;
        double s_mouseDeltaX = 0.0;
        double s_mouseDeltaY = 0.0;
        bool s_hasLastMousePosition = false;
    }

    void Input::Update(GLFWwindow* window)
    {
        s_keysPressed.reset();
        s_buttonsPressed.reset();
        s_mouseDeltaX = 0.0;
        s_mouseDeltaY = 0.0;

        if (window == nullptr)
        {
            return;
        }

        // GLFW key codes start at GLFW_KEY_SPACE (32); lower values are unused.
        for (int key = GLFW_KEY_SPACE; key < KeyCount; ++key)
        {
            const bool down = glfwGetKey(window, key) == GLFW_PRESS;
            s_keysPressed[key] = down && !s_keysDown[key];
            s_keysDown[key] = down;
        }

        for (int button = 0; button < MouseButtonCount; ++button)
        {
            const bool down = glfwGetMouseButton(window, button) == GLFW_PRESS;
            s_buttonsPressed[button] = down && !s_buttonsDown[button];
            s_buttonsDown[button] = down;
        }

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (s_hasLastMousePosition)
        {
            s_mouseDeltaX = mouseX - s_lastMouseX;
            s_mouseDeltaY = mouseY - s_lastMouseY;
        }

        s_lastMouseX = mouseX;
        s_lastMouseY = mouseY;
        s_hasLastMousePosition = true;
    }

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

    bool Input::WasKeyPressed(int key)
    {
        return key >= 0 && key < KeyCount && s_keysPressed[key];
    }

    bool Input::WasMouseButtonPressed(int button)
    {
        return button >= 0 && button < MouseButtonCount && s_buttonsPressed[button];
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

    void Input::GetMouseDelta(double& deltaX, double& deltaY)
    {
        deltaX = s_mouseDeltaX;
        deltaY = s_mouseDeltaY;
    }

    void Input::SetCursorCaptured(GLFWwindow* window, bool captured)
    {
        if (window == nullptr)
        {
            return;
        }

        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        // Raw motion skips OS pointer acceleration, which feels wrong for
        // camera control.
        if (glfwRawMouseMotionSupported() == GLFW_TRUE)
        {
            glfwSetInputMode(
                window,
                GLFW_RAW_MOUSE_MOTION,
                captured ? GLFW_TRUE : GLFW_FALSE);
        }

        // The cursor position jumps when the mode changes. Skip one frame of
        // delta so the camera does not snap.
        s_hasLastMousePosition = false;
    }

    bool Input::IsCursorCaptured(GLFWwindow* window)
    {
        return window != nullptr &&
            glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    }
}
