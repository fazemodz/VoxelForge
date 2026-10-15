#pragma once

#include "../Maths/Math.h"

namespace VoxelForge
{
    // Perspective camera described by a position and yaw/pitch angles.
    // World space is right-handed with +Y up. Yaw 0 looks down -Z.
    class Camera
    {
    public:
        Vec3 position{ 0.0f, 0.0f, 0.0f };
        float yawDegrees = 0.0f;
        float pitchDegrees = 0.0f;

        float fieldOfViewDegrees = 70.0f;
        float nearPlane = 0.05f;
        float farPlane = 1000.0f;

        Vec3 GetForward() const;
        Vec3 GetRight() const;

        // Forward direction flattened onto the ground plane, for walking.
        Vec3 GetFlatForward() const;

        Mat4 GetViewMatrix() const;

        // Uses Vulkan's 0..1 depth range and flips Y so +Y is up on screen.
        Mat4 GetProjectionMatrix(float aspectRatio) const;

        // Turns the camera and keeps the pitch short of straight up/down,
        // where the view matrix would flip.
        void Rotate(float yawDeltaDegrees, float pitchDeltaDegrees);
    };
}
