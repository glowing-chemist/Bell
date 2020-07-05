#ifndef RAY_TRACING_SCENE_HPP
#define RAY_TRACING_SCENE_HPP

//#define NANORT_USE_CPP11_FEATURE 1
#include "ThirdParty/nanort/nanort.h"

#include "GeomUtils.h"
#include "CPUImage.hpp"

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

    struct InterpolatedVertex
    {
        float4 mPosition;
        float2 mUV;
        float4 mNormal;
        float4 mVertexColour;
    };
    InterpolatedVertex interpolateFragment(const uint32_t primID, const float u, const float v);

private:

    std::vector<float3> mPositions;
    std::vector<float2> mUVs;
    std::vector<float4> mNormals;
    std::vector<float4> mVertexColours;
    std::vector<uint32_t> mIndexBuffer;

    std::unique_ptr<nanort::TriangleMesh<float>> mMeshes;
    std::unique_ptr<nanort::TriangleSAHPred<float>> mPred;

    nanort::BVHAccel<float> mAccelerationStructure;

    struct MaterialInfo
    {
        uint32_t materialIndex;
        uint32_t materialFlags;
    };
    std::vector<MaterialInfo> mPrimitiveMaterialID; // maps prim ID to material ID.
    std::vector<CPUImage> mMaterials;
};


#endif
