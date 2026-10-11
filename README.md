\# VoxelForge



VoxelForge is a custom voxel game engine written in C++20. The goal is to build a Minecraft-style sandbox game while developing the engine and its core systems from the ground up.



The project uses Vulkan for graphics and GLFW for window creation and input. It is developed in Visual Studio on Windows.



\## Features



\### Current

\- C++20 project setup

\- GLFW window creation

\- Keyboard and mouse input queries

\- Main application loop

\- Frame timing with delta time

\- Window event handling

\- Clean application shutdown

\- Solution-root-relative dependency paths



\### Planned

\- Vulkan instance and device management

\- Vulkan rendering pipeline

\- 3D camera and shader systems

\- Voxel block definitions

\- Chunk storage and mesh generation

\- Procedural terrain generation

\- First-person player movement and collision

\- Block mining and placement

\- Inventory and crafting

\- Chunk streaming and world saving

\- Lighting, biomes and other world features



\## Technology stack



\- \*\*Language:\*\* C++20

\- \*\*IDE:\*\* Visual Studio

\- \*\*Graphics API:\*\* Vulkan

\- \*\*Window and input:\*\* GLFW

\- \*\*Build system:\*\* Visual Studio solution and project files

\- \*\*Platform:\*\* Windows x64



\## Project structure



```text

VoxelForge/

├── Dependencies/

│   └── GLFW/

│       ├── include/

│       └── lib-vc2022/

├── VoxelForge/

│   ├── Source/

│   │   ├── main.cpp

│   │   └── Engine/

│   │       └── Core/

│   │           ├── Application.h

│   │           ├── Application.cpp

│   │           ├── Window.h

│   │           ├── Window.cpp

│   │           ├── Input.h

│   │           ├── Input.cpp

│   │           ├── Time.h

│   │           └── Time.cpp

│   └── VoxelForge.vcxproj

├── VoxelForge.sln

└── README.md

```



\## Requirements



\- Visual Studio with the Desktop development with C++ workload

\- A compatible MSVC C++ compiler

\- Vulkan SDK

\- GLFW Windows libraries for Visual C++

\- A compatible graphics driver



\## Setup



1\. Clone or download the repository.

2\. Install Visual Studio and the Vulkan SDK.

3\. Place the GLFW headers and libraries under `Dependencies/GLFW/`.

4\. Open `VoxelForge.sln` in Visual Studio.

5\. Select the `Debug` configuration and `x64` platform.

6\. Build the solution.

7\. Run the `VoxelForge` project.



The project uses `$(SolutionDir)` for dependency paths so that the repository does not rely on a specific Windows username or absolute project location.



Ensure the GLFW library and DLL filenames match those referenced in the project settings.



\## Controls



| Key | Action |

|---|---|

| Escape | Close the application |



\## Development status



VoxelForge is in its early development stage. The current focus is the engine foundation, including window creation, input handling, timing and application lifecycle management.



Vulkan rendering and gameplay systems have not yet been implemented.



\## License



The project license has not yet been defined. Add a license before distributing the project publicly.



