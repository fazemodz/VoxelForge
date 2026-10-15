#include "Application.h"

#include "Input.h"
#include "Time.h"
#include "Renderer/Geometry/Cube.h"

#include <glfw3.h>
#include <iostream>

namespace VoxelForge
{
    namespace
    {
        // Size of the test floor, in blocks along each side.
        constexpr int FloorSize = 32;
    }

    bool Application::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        std::cout << "Starting VoxelForge...\n";

        if (!m_window.Create("VoxelForge", 1280, 720))
        {
            std::cerr << "Failed to initialize VoxelForge window.\n";
            return false;
        }

        // Initialise Vulkan after GLFW has created the window.
        if (!m_vulkanContext.Initialize(m_window.GetNativeWindow()))
        {
            std::cerr << "Failed to initialize Vulkan.\n";
            m_vulkanContext.Shutdown();
            return false;
        }

        if (!m_cubeMesh.Create(
            m_vulkanContext.GetUploadContext(),
            Cube::Vertices,
            Cube::Indices))
        {
            std::cerr << "Failed to create cube mesh.\n";
            m_vulkanContext.Shutdown();
            return false;
        }

        // Start above the floor, looking slightly down at the test cube.
        m_camera.position = Vec3(0.0f, 3.0f, 8.0f);
        m_camera.pitchDegrees = -15.0f;

        Time::Reset();

        m_initialized = true;
        std::cout << "VoxelForge initialized.\n";
        std::cout << "Controls: click to capture the mouse, WASD to move, "
            << "Space/Shift up/down, Ctrl to fly faster, Esc to release the mouse "
            << "(Esc again quits).\n";

        return true;
    }

    int Application::Run()
    {
        if (!m_initialized)
        {
            std::cerr << "Cannot run: application is not initialized.\n";
            return 1;
        }

        std::cout << "Entering main loop.\n";

        while (!m_window.ShouldClose())
        {
            m_window.PollEvents();
            Time::Tick();
            Input::Update(m_window.GetNativeWindow());

            Update(Time::GetDeltaTime());
            SubmitScene();

            m_vulkanContext.RenderFrame(m_camera);
        }

        std::cout << "Leaving main loop.\n";

        // Let the GPU finish the last frames, then destroy GPU resources
        // before the Vulkan device, and Vulkan before the window.
        m_vulkanContext.WaitIdle();
        m_cubeMesh.Destroy();
        m_vulkanContext.Shutdown();

        m_initialized = false;
        return 0;
    }

    void Application::Update(float deltaTime)
    {
        GLFWwindow* nativeWindow = m_window.GetNativeWindow();
        const bool captured = Input::IsCursorCaptured(nativeWindow);

        if (Input::WasKeyPressed(GLFW_KEY_ESCAPE))
        {
            if (captured)
            {
                Input::SetCursorCaptured(nativeWindow, false);
            }
            else
            {
                m_window.RequestClose();
            }
        }
        else if (!captured && Input::WasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            Input::SetCursorCaptured(nativeWindow, true);
        }

        m_cameraController.Update(m_camera, nativeWindow, deltaTime);
    }

    void Application::SubmitScene()
    {
        // A flat floor of blocks gives a sense of scale and motion until
        // chunks exist. Cube vertices span -0.5..0.5, so offset by 0.5 to put
        // block corners on whole numbers like Minecraft.
        const int halfSize = FloorSize / 2;

        for (int x = -halfSize; x < halfSize; ++x)
        {
            for (int z = -halfSize; z < halfSize; ++z)
            {
                const Mat4 model = glm::translate(
                    Mat4(1.0f),
                    Vec3(x + 0.5f, -0.5f, z + 0.5f));

                m_vulkanContext.Submit(m_cubeMesh, model);
            }
        }

        // The original spinning cube, now floating above the floor.
        Mat4 spinning = glm::translate(Mat4(1.0f), Vec3(0.0f, 1.5f, 0.0f));
        spinning = glm::rotate(
            spinning,
            Time::GetElapsedTime() * glm::radians(35.0f),
            Vec3(0.0f, 1.0f, 0.0f));

        m_vulkanContext.Submit(m_cubeMesh, spinning);
    }
}
