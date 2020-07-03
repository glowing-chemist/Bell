#include "Engine/RayTracedScene.hpp"

#include "Engine/Scene.h"

#include <algorithm>

#include "stbi_image_write.h"


RayTracingScene::RayTracingScene(const Scene* scene)
{
    uint64_t vertexOffset = 0;
    for(const auto& instance : scene->getStaticMeshInstances())
    {
        mVertexStride = instance.mMesh->getVertexStride();

        // add Index data.
        const auto& indexBuffer = instance.mMesh->getIndexData();
        std::transform(indexBuffer.begin(), indexBuffer.end(), std::back_inserter(mIndexBuffer), [vertexOffset](const uint32_t index)
        {
            return vertexOffset + index;
        });

        // Transform and add vertex data.
        const auto& vertexData = instance.mMesh->getVertexData();
        const size_t oldVertexBufferSize = mVertexBuffer.size();
        mVertexBuffer.resize(oldVertexBufferSize + vertexData.size());
        for(uint32_t i = 0; i < vertexData.size(); i += mVertexStride)
        {
            const unsigned char* vert = &vertexData[i];
            const float* positionPtr = reinterpret_cast<const float*>(vert);

            const float4 position = float4{positionPtr[0], positionPtr[1], positionPtr[2], positionPtr[3]};
            BELL_ASSERT(position.w == 1.0f, "Probably incorrect pointer")
            const float4 transformedPosition = instance.getTransMatrix() * position;
            BELL_ASSERT(transformedPosition.w == 1.0f, "Non afine transform")

            *reinterpret_cast<float*>(&mVertexBuffer[oldVertexBufferSize] + i) = transformedPosition.x;
            *reinterpret_cast<float*>(&mVertexBuffer[oldVertexBufferSize] + i + sizeof(float)) = transformedPosition.y;
            *reinterpret_cast<float*>(&mVertexBuffer[oldVertexBufferSize] + i + 2 * sizeof(float)) = transformedPosition.z;
            *reinterpret_cast<float*>(&mVertexBuffer[oldVertexBufferSize] + i + 3 * sizeof(float)) = 1.0f;

            std::memcpy(&mVertexBuffer[oldVertexBufferSize] + i + sizeof(float) * 4, vert + (sizeof(float) * 4), mVertexStride - (sizeof(float) * 4));
        }
        //mVertexBuffer.insert(mVertexBuffer.end(), vertexData.begin(), vertexData.end());

        vertexOffset += instance.mMesh->getVertexCount();
    }

    mMeshes = std::make_unique<nanort::TriangleMesh<float>>(reinterpret_cast<float*>(mVertexBuffer.data()), mIndexBuffer.data(), mVertexStride);
    mPred = std::make_unique<nanort::TriangleSAHPred<float>>(reinterpret_cast<float*>(mVertexBuffer.data()), mIndexBuffer.data(), mVertexStride);

    BELL_ASSERT((mIndexBuffer.size() % 3) == 0, "Invalid index buffer size")
    bool success = mAccelerationStructure.Build(mIndexBuffer.size() / 3, *mMeshes, *mPred);
    BELL_ASSERT(success, "Failed to build BVH")
}

#pragma pack(1)
struct PackedVertex
{
    float4 position;
    float2 uv;
    float4 normals;
    uint albedo;
};
#pragma pack()

void RayTracingScene::renderSceneToMemory(const Camera& camera, const uint32_t x, const uint32_t y, uint8_t* memory)
{
    float3 forward = camera.getDirection();
    float3 up = camera.getUp();
    float3 right = camera.getRight();

    float3 origin = camera.getPosition();
    float far = camera.getFarPlane();

    for(uint32_t i = 0; i < x; ++i)
    {
        for(uint32_t j = 0; j < y; ++j)
        {
            float3 dir = {(float(i) / float(x)) - 0.5f, (float(j) / float(y)) - 0.5f, 1.0f};
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

            nanort::TriangleIntersector triangle_intersecter(reinterpret_cast<float*>(mVertexBuffer.data()), mIndexBuffer.data(), mVertexStride);
            nanort::TriangleIntersection intersection;
            const bool hit = mAccelerationStructure.Traverse(ray, triangle_intersecter, &intersection);
            if(hit)
            {
                const PackedVertex* first = reinterpret_cast<const PackedVertex*>(mVertexBuffer.data() + (intersection.prim_id * mVertexStride));
                memory[(i + (j * y)) * 4] = first->normals.x * 255.0f;
                memory[((i + (j * y)) * 4) + 1] = first->normals.y * 255/.0f;
                memory[((i + (j * y)) * 4) + 2] = first->normals.z * 255.0f;
                memory[((i + (j * y)) * 4) + 3] = 255;
            }
        }
    }
}


void RayTracingScene::renderSceneToFile(const Camera& camera, const uint32_t x, const uint32_t y, const char* path)
{
    uint8_t* memory = new uint8_t[x * y * 4];
    memset(memory, 0, x * y * 4);

    renderSceneToMemory(camera, x, y, memory);

    stbi_write_jpg(path, x, y, 4, memory, 100);

    delete[] memory;
}
