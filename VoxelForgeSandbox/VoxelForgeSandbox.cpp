
#include <iostream>

#include "Engine/Core/Application.h"
using namespace VoxelForge;

int main()
{
    std::cout << "Starting VoxelForge Sandbox...\n";

    Application application;

    if (!application.Initialize())
    {
        std::cerr << "Failed to initialize VoxelForge.\n";
        return 1;
    }

    std::cout << "VoxelForge Sandbox initialized.\n";

    application.Run();

    std::cout << "VoxelForge Sandbox shutting down.\n";
    return 0;
}
