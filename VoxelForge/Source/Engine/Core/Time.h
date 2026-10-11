#pragma once

#include <chrono>

namespace VoxelForge
{
    class Time
    {
    public:
        static void Reset();
        static void Tick();

        static float GetDeltaTime();
        static float GetElapsedTime();

    private:
        using Clock = std::chrono::steady_clock;

        static Clock::time_point s_startTime;
        static Clock::time_point s_lastFrameTime;

        static float s_deltaTime;
        static float s_elapsedTime;
    };
}