#ifndef RAY_TRACING_SCENE_HPP
#define RAY_TRACING_SCENE_HPP

#define NANORT_USE_CPP11_FEATURE 1
#include "ThirdParty/nanort/nanort.h"

#include <memory>
#include <vector>

class Scene;
class Camera;

class RayTracingScene
{
public:

    RayTracingScene(const Scene*);
    ~RayTracingScene() = default;

    void renderSceneToMemory(const Camera&, const uint32_t x, const uint32_t y, uint8_t *);
    void renderSceneToFile(const Camera&, const uint32_t x, const uint32_t y, const char*);

private:

    uint32_t mVertexStride;
    std::vector<unsigned char> mVertexBuffer;
    std::vector<uint32_t> mIndexBuffer;

    std::unique_ptr<nanort::TriangleMesh<float>> mMeshes;
    std::unique_ptr<nanort::TriangleSAHPred<float>> mPred;

    nanort::BVHAccel<float> mAccelerationStructure;
};


#endif
