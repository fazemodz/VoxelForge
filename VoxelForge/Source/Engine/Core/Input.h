#pragma once

struct GLFWwindow;

namespace VoxelForge
{
    class Input
    {
    public:
        static bool IsKeyDown(
            GLFWwindow* window,
            int key
        );

        static bool IsMouseButtonDown(
            GLFWwindow* window,
            int button
        );

        static void GetMousePosition(
            GLFWwindow* window,
            double& x,
            double& y
        );
    };
}