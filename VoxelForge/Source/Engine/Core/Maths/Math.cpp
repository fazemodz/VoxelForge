
#include "Math.h"

namespace VoxelForge
{
    void TestMath()
    {
        Vec3 position(1.0f, 2.0f, 3.0f);
        Mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, position);
    }
}
