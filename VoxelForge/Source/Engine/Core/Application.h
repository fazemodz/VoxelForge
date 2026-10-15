#pragma once

#include "FlyCameraController.h"
#include "Window.h"
#include "Renderer/Camera.h"
#include "Renderer/Mesh.h"
#include "Renderer/VulkanContext.h"

namespace VoxelForge
{
    class Application
    {
    public:
        bool Initialize();
        int Run();

    private:
        void Update(float deltaTime);
        void SubmitScene();

        Window m_window;
        VulkanContext m_vulkanContext;

        // Declared after the Vulkan context so it is destroyed first.
        Mesh m_cubeMesh;

        Camera m_camera;
        FlyCameraController m_cameraController;

        bool m_initialized = false;
    };
}
