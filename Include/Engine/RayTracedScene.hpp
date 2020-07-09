#ifndef RAY_TRACING_SCENE_HPP
#define RAY_TRACING_SCENE_HPP

//#define NANORT_USE_CPP11_FEATURE 1
#include "ThirdParty/nanort/nanort.h"

#include "GeomUtils.h"
#include "CPUImage.hpp"
#include "Engine/RayTracingSamplers.hpp"

#include <memory>
#include <vector>

class Scene;
class Camera;

class RayTracingScene
{
public:

    RayTracingScene(const Scene*);
    ~RayTracingScene() = default;

    void renderSceneToMemory(const Camera&, const uint32_t x, const uint32_t y, uint8_t *) const;
    void renderSceneToFile(const Camera&, const uint32_t x, const uint32_t y, const char*) const;

    struct InterpolatedVertex
    {
        float4 mPosition;
        float2 mUV;
        float4 mNormal;
        float4 mVertexColour;
        uint32_t mPrimID;
    };
    InterpolatedVertex interpolateFragment(const uint32_t primID, const float u, const float v) const;

    struct MaterialInfo
    {
        uint32_t materialIndex;
        uint32_t materialFlags;
    };

    struct Material
    {
        float4 diffuse;
        float4 specularRoughness; // xyz specular w roughness
        float4 normal;
        float4 emissiveOcclusion; // xyz emisive w ambient occlusion.
    };
    Material calculateMaterial(const InterpolatedVertex&, const MaterialInfo&, const float3 &view) const;

private:

    bool traceRay(const nanort::Ray<float>& ray, InterpolatedVertex* result) const;

    bool traceShadowRay(const InterpolatedVertex& position) const;

    float4 traceDiffuseRays(const InterpolatedVertex& frag, const float4 &origin, const uint32_t sampleCount, const uint32_t depth) const;
    float4 traceDiffuseRay(DiffuseSampler& sampler, const InterpolatedVertex& frag, const float4 &origin, const uint32_t depth) const;

    std::vector<float3> mPositions;
    std::vector<float2> mUVs;
    std::vector<float4> mNormals;
    std::vector<float4> mVertexColours;
    std::vector<uint32_t> mIndexBuffer;

    std::unique_ptr<nanort::TriangleMesh<float>> mMeshes;
    std::unique_ptr<nanort::TriangleSAHPred<float>> mPred;

    nanort::BVHAccel<float> mAccelerationStructure;

    std::vector<MaterialInfo> mPrimitiveMaterialID; // maps prim ID to material ID.
    const Scene* mScene;
};


#endif
