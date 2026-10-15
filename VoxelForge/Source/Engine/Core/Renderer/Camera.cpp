#include "Camera.h"

#include <glm/ext/matrix_clip_space.hpp>

#include <algorithm>
#include <cmath>

namespace VoxelForge
{
    namespace
    {
        constexpr Vec3 WorldUp{ 0.0f, 1.0f, 0.0f };
        constexpr float MaxPitchDegrees = 89.0f;
    }

    Vec3 Camera::GetForward() const
    {
        const float yaw = glm::radians(yawDegrees);
        const float pitch = glm::radians(pitchDegrees);

        return glm::normalize(Vec3(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            -std::cos(yaw) * std::cos(pitch)));
    }

    Vec3 Camera::GetRight() const
    {
        return glm::normalize(glm::cross(GetForward(), WorldUp));
    }

    Vec3 Camera::GetFlatForward() const
    {
        const float yaw = glm::radians(yawDegrees);
        return Vec3(std::sin(yaw), 0.0f, -std::cos(yaw));
    }

    Mat4 Camera::GetViewMatrix() const
    {
        return glm::lookAt(position, position + GetForward(), WorldUp);
    }

    Mat4 Camera::GetProjectionMatrix(float aspectRatio) const
    {
        Mat4 projection = glm::perspectiveRH_ZO(
            glm::radians(fieldOfViewDegrees),
            aspectRatio,
            nearPlane,
            farPlane);

        // Vulkan's clip space has +Y pointing down.
        projection[1][1] *= -1.0f;
        return projection;
    }

    void Camera::Rotate(float yawDeltaDegrees, float pitchDeltaDegrees)
    {
        yawDegrees = std::fmod(yawDegrees + yawDeltaDegrees, 360.0f);
        pitchDegrees = std::clamp(
            pitchDegrees + pitchDeltaDegrees,
            -MaxPitchDegrees,
            MaxPitchDegrees);
    }
}
