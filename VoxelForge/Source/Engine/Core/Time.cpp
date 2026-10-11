#include "Time.h"

namespace VoxelForge
{
    Time::Clock::time_point Time::s_startTime{};
    Time::Clock::time_point Time::s_lastFrameTime{};

    float Time::s_deltaTime = 0.0f;
    float Time::s_elapsedTime = 0.0f;

    void Time::Reset()
    {
        s_startTime = Clock::now();
        s_lastFrameTime = s_startTime;

        s_deltaTime = 0.0f;
        s_elapsedTime = 0.0f;
    }

    void Time::Tick()
    {
        const auto now = Clock::now();

        s_deltaTime =
            std::chrono::duration<float>(
                now - s_lastFrameTime
            ).count();

        s_elapsedTime =
            std::chrono::duration<float>(
                now - s_startTime
            ).count();

        s_lastFrameTime = now;
    }

    float Time::GetDeltaTime()
    {
        return s_deltaTime;
    }

    float Time::GetElapsedTime()
    {
        return s_elapsedTime;
    }
}