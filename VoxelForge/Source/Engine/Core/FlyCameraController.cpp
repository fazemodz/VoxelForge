#include "FlyCameraController.h"

#include "Input.h"
#include "Renderer/Camera.h"

#include <glfw3.h>

namespace VoxelForge
{
    void FlyCameraController::Update(Camera& camera, GLFWwindow* window, float deltaTime) const
    {
        if (window == nullptr)
        {
            return;
        }

        if (Input::IsCursorCaptured(window))
        {
            double deltaX = 0.0;
            double deltaY = 0.0;
            Input::GetMouseDelta(deltaX, deltaY);

            // Screen Y grows downward, so moving the mouse up looks up.
            camera.Rotate(
                static_cast<float>(deltaX) * mouseSensitivity,
                static_cast<float>(-deltaY) * mouseSensitivity);
        }

        const Vec3 forward = camera.GetFlatForward();
        const Vec3 right = camera.GetRight();
        const Vec3 up{ 0.0f, 1.0f, 0.0f };

        Vec3 direction{ 0.0f };

        if (Input::IsKeyDown(window, GLFW_KEY_W)) direction += forward;
        if (Input::IsKeyDown(window, GLFW_KEY_S)) direction -= forward;
        if (Input::IsKeyDown(window, GLFW_KEY_D)) direction += right;
        if (Input::IsKeyDown(window, GLFW_KEY_A)) direction -= right;
        if (Input::IsKeyDown(window, GLFW_KEY_SPACE)) direction += up;
        if (Input::IsKeyDown(window, GLFW_KEY_LEFT_SHIFT)) direction -= up;

        if (glm::dot(direction, direction) == 0.0f)
        {
            return;
        }

        // Normalise so moving diagonally is not faster.
        float speed = moveSpeed;
        if (Input::IsKeyDown(window, GLFW_KEY_LEFT_CONTROL))
        {
            speed *= sprintMultiplier;
        }

        camera.position += glm::normalize(direction) * speed * deltaTime;
    }
}
