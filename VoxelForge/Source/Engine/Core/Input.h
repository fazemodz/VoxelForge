#pragma once

struct GLFWwindow;

namespace VoxelForge
{
    class Input
    {
    public:
        // Call once per frame after polling events. Records key and mouse
        // button state so the "pressed this frame" queries work, and measures
        // how far the mouse moved since the previous frame.
        static void Update(GLFWwindow* window);

        static bool IsKeyDown(
            GLFWwindow* window,
            int key
        );

        static bool IsMouseButtonDown(
            GLFWwindow* window,
            int button
        );

        // True only on the frame the key or button went down.
        static bool WasKeyPressed(int key);
        static bool WasMouseButtonPressed(int button);

        static void GetMousePosition(
            GLFWwindow* window,
            double& x,
            double& y
        );

        // Mouse movement in pixels since the previous Update.
        static void GetMouseDelta(double& deltaX, double& deltaY);

        // A captured cursor is hidden and locked to the window, giving
        // unlimited mouse movement for looking around.
        static void SetCursorCaptured(GLFWwindow* window, bool captured);
        static bool IsCursorCaptured(GLFWwindow* window);
    };
}
