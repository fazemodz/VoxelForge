#pragma once

struct GLFWwindow;

namespace VoxelForge
{
    class Camera;

    // Creative-mode style flying: WASD moves along the ground plane,
    // Space/Left Shift move up/down, Left Ctrl speeds up. The mouse turns
    // the camera while the cursor is captured.
    class FlyCameraController
    {
    public:
        float moveSpeed = 8.0f;           // blocks per second
        float sprintMultiplier = 3.0f;
        float mouseSensitivity = 0.1f;    // degrees per pixel

        void Update(Camera& camera, GLFWwindow* window, float deltaTime) const;
    };
}
