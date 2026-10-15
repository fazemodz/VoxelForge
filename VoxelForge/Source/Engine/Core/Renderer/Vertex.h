#pragma once

namespace VoxelForge
{
    // Layout must match the vertex input description in the graphics pipeline
    // and the inputs of the vertex shader.
    struct Vertex
    {
        float position[3];
        float color[3];
    };
}
