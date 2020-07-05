#include "Engine/RayTracedScene.hpp"

#include "Engine/Scene.h"
#include "Core/ConversionUtils.hpp"

#include <algorithm>
#include <thread>

#include "stbi_image_write.h"


RayTracingScene::RayTracingScene(const Scene* scene)
{
    mScene = scene;
    uint64_t vertexOffset = 0;
    for(const auto& instance : scene->getStaticMeshInstances())
    {
        const uint32_t vertexStride = instance.mMesh->getVertexStride();

        // add Index data.
        const auto& indexBuffer = instance.mMesh->getIndexData();
        std::transform(indexBuffer.begin(), indexBuffer.end(), std::back_inserter(mIndexBuffer), [vertexOffset](const uint32_t index)
        {
            return vertexOffset + index;
        });

        // add material mappings
        for(uint32_t i = 0; i < indexBuffer.size(); i += 3)
        {
            mPrimitiveMaterialID.push_back({instance.getmaterialIndex(), instance.getMaterialFlags()});
        }

        // Transform and add vertex data.
        const auto& vertexData = instance.mMesh->getVertexData();
        for(uint32_t i = 0; i < vertexData.size(); i += vertexStride)
        {
            const unsigned char* vert = &vertexData[i];
            const float* positionPtr = reinterpret_cast<const float*>(vert);

            const float4 position = float4{positionPtr[0], positionPtr[1], positionPtr[2], positionPtr[3]};
            BELL_ASSERT(position.w == 1.0f, "Probably incorrect pointer")
            const float4 transformedPosition = instance.getTransMatrix() * position;
            BELL_ASSERT(transformedPosition.w == 1.0f, "Non afine transform")

            mPositions.emplace_back(transformedPosition.x, transformedPosition.y, transformedPosition.z);

            const float2 uv = float2{positionPtr[4], positionPtr[5]};
            mUVs.push_back(uv);

            const float4 normal = float4{positionPtr[6], positionPtr[7], positionPtr[8], positionPtr[9]};
            mNormals.push_back(normal);

            const float4 colour = unpackColour(*reinterpret_cast<const uint32_t*>(&positionPtr[10]));
            mVertexColours.push_back(colour);
        }

        vertexOffset += instance.mMesh->getVertexCount();
    }

    mMeshes = std::make_unique<nanort::TriangleMesh<float>>(reinterpret_cast<float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));
    mPred = std::make_unique<nanort::TriangleSAHPred<float>>(reinterpret_cast<float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));

    BELL_ASSERT((mIndexBuffer.size() % 3) == 0, "Invalid index buffer size")
    bool success = mAccelerationStructure.Build(mIndexBuffer.size() / 3, *mMeshes, *mPred);
    BELL_ASSERT(success, "Failed to build BVH")
}

void RayTracingScene::renderSceneToMemory(const Camera& camera, const uint32_t x, const uint32_t y, uint8_t* memory)
{
    const float3 forward = camera.getDirection();
    const float3 up = camera.getUp();
    const float3 right = camera.getRight();

    const float3 origin = camera.getPosition();
    const float far = camera.getFarPlane();
    const float aspect = camera.getAspect();

    const std::vector<CPUImage>& materials = mScene->getCPUImageMaterials();

    auto trace_ray = [&](const uint32_t pix, const uint32_t piy) -> float4
    {
        float3 dir = {((float(pix) / float(x)) - 0.5f) * aspect, (float(piy) / float(y)) - 0.5f, 1.0f};
        dir = glm::normalize((dir.z * forward) + (dir.y * up) + (dir.x * right));

        nanort::Ray<float> ray;
        ray.dir[0] = dir.x;
        ray.dir[1] = dir.y;
        ray.dir[2] = dir.z;

        ray.org[0] = origin.x;
        ray.org[1] = origin.y;
        ray.org[2] = origin.z;

        ray.min_t = 0.0f;
        ray.max_t = far;
        //ray.type = nanort::RAY_TYPE_PRIMARY;

        InterpolatedVertex frag;
        const bool hit = traceRay(ray, &frag);
        if(hit)
        {
            // interpolate uvs.
            MaterialInfo& matInfo = mPrimitiveMaterialID[frag.mPrimID];

            float4 diffuse = frag.mVertexColour;
            if(matInfo.materialFlags & MaterialType::Albedo || matInfo.materialFlags & MaterialType::Diffuse)
            {
                const CPUImage& diffuseTexture = materials[matInfo.materialIndex];
                const float4 colour = diffuseTexture.sample4(frag.mUV);

                diffuse = colour;
            }

            const bool shadowed = traceShadowRay(frag);

            return diffuse * (shadowed ? 0.15f : 1.0f);
        }

        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    };

    auto trace_rays = [&](const uint32_t start, const uint32_t stepSize)
    {
        uint32_t location = start;
        while(location < (x * y))
        {
            const uint32_t pix = location % x;
            const uint32_t piy = location / x;
            const uint32_t colour = packColour(trace_ray(pix, piy));
            memcpy(&memory[(pix + (piy * x)) * 4], &colour, sizeof(uint32_t));

            location += stepSize;
        }
    };

    const uint32_t processor_count = std::thread::hardware_concurrency(); // use this many threads for tracing rays.
    std::vector<std::thread> helperThreads{};
    for(uint32_t i = 1; i < processor_count; ++i)
    {
        helperThreads.push_back(std::thread(trace_rays, i, processor_count));
    }

    trace_rays(0, processor_count);

    for(auto& thread : helperThreads)
        thread.join();

    return;
}


void RayTracingScene::renderSceneToFile(const Camera& camera, const uint32_t x, const uint32_t y, const char* path)
{
    uint8_t* memory = new uint8_t[x * y * 4];
    BELL_ASSERT(memory, "Unable to allocate memory")
    memset(memory, 0, x * y * 4);

    renderSceneToMemory(camera, x, y, memory);

    stbi_write_jpg(path, x, y, 4, memory, 100);

    delete[] memory;
}


RayTracingScene::InterpolatedVertex RayTracingScene::interpolateFragment(const uint32_t primID, const float u, const float v)
{
    BELL_ASSERT((u + v) <= 1.0f, "Out of bounds")

    const uint32_t baseIndiciesIndex = primID * 3;
    BELL_ASSERT(primID * 3 < mIndexBuffer.size(), "PrimID out of bounds")

    const uint32_t firstIndex = mIndexBuffer[baseIndiciesIndex];
    const float3& firstPosition = mPositions[firstIndex];
    const float2& firstuv = mUVs[firstIndex];
    const float4& firstNormal = mNormals[firstIndex];
    const float4 firstColour = mVertexColours[firstIndex];

    const uint32_t secondIndex = mIndexBuffer[baseIndiciesIndex + 1];
    const float3& secondPosition = mPositions[secondIndex];
    const float2& seconduv = mUVs[secondIndex];
    const float4& secondNormal = mNormals[secondIndex];
    const float4& secondColour = mVertexColours[secondIndex];

    const uint32_t thirdIndex = mIndexBuffer[baseIndiciesIndex + 2];
    const float3& thirdPosition = mPositions[thirdIndex];
    const float2& thirduv = mUVs[thirdIndex];
    const float4& thirdNormal = mNormals[thirdIndex];
    const float4& thirdColour = mVertexColours[thirdIndex];

    InterpolatedVertex frag{};
    frag.mPosition = float4(((1.0f - v - u) * firstPosition) + (u * secondPosition) + (v * thirdPosition), 1.0f);
    frag.mUV = ((1.0f - v - u) * firstuv) + (u * seconduv) + (v * thirduv);
    frag.mNormal = glm::normalize(((1.0f - v - u) * firstNormal) + (u * secondNormal) + (v * thirdNormal));
    frag.mVertexColour = ((1.0f - v - u) * firstColour) + (u * secondColour) + (v * thirdColour);
    frag.mPrimID = primID;

    return frag;
}


bool RayTracingScene::traceRay(const nanort::Ray<float>& ray, InterpolatedVertex *result)
{
    nanort::TriangleIntersector triangle_intersecter(reinterpret_cast<float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));
    nanort::TriangleIntersection intersection;
    bool hit = mAccelerationStructure.Traverse(ray, triangle_intersecter, &intersection);

    if(hit) // check for alpha tested geometry.
    {
        *result= interpolateFragment(intersection.prim_id, intersection.u, intersection.v);

        const std::vector<CPUImage>& materials = mScene->getCPUImageMaterials();
        MaterialInfo& matInfo = mPrimitiveMaterialID[result->mPrimID];

        if(matInfo.materialFlags & MaterialType::Albedo || matInfo.materialFlags & MaterialType::Diffuse)
        {

            const CPUImage& diffuseTexture = materials[matInfo.materialIndex];
            const float4 colour = diffuseTexture.sample4(result->mUV);

            if(colour.a == 0.0f) // trace another ray.
            {
                nanort::Ray<float> newRay{};
                newRay.org[0] = result->mPosition.x;
                newRay.org[1] = result->mPosition.y;
                newRay.org[2] = result->mPosition.z;
                newRay.dir[0] = ray.dir[0];
                newRay.dir[1] = ray.dir[1];
                newRay.dir[2] = ray.dir[2];
                newRay.min_t = 0.01f;
                newRay.max_t = 2000.0f;

                hit = traceRay(newRay, result);
            }
        }
    }

    return hit;
}


bool RayTracingScene::traceShadowRay(const InterpolatedVertex& position)
{
    const Scene::ShadowingLight& sun = mScene->getShadowingLight();

    float4 dir = glm::normalize(sun.mPosition - position.mPosition);

    nanort::Ray<float> shadowRay{};
    shadowRay.dir[0] = dir.x;
    shadowRay.dir[1] = dir.y;
    shadowRay.dir[2] = dir.z;
    shadowRay.org[0] = position.mPosition.x;
    shadowRay.org[1] = position.mPosition.y;
    shadowRay.org[2] = position.mPosition.z;
    shadowRay.min_t = 0.01f;
    shadowRay.max_t = 2000.0f;

    InterpolatedVertex intersection;
    return traceRay(shadowRay, &intersection);
}
