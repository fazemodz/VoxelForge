#include "Engine/Core/Application.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        VoxelForge::Application application;

        if (!application.Initialize())
        {
            return 1;
        }

        return application.Run();
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "[Fatal] "
            << exception.what()
            << '\n';

        return 1;
    }
    catch (...)
    {
        std::cerr << "[Fatal] Unknown exception.\n";
        return 1;
    }
}