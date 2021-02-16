#include "Engine/RayTracedScene.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"
#include "PBR.hpp"
#include "Core/ConversionUtils.hpp"

#include <algorithm>
#include <thread>

#include "stbi_image_write.h"


RayTracingScene::RayTracingScene(RenderEngine* eng, const Scene* scene) :
    mPrimitiveMaterialID{},
    mBVH_SRS{eng->getDevice(), 6}
{
    updateCPUAccelerationStructure(scene);
    
    nanort::BVHBuildStatistics stats = mAccelerationStructure.GetStatistics();
    BELL_ASSERT(stats.max_tree_depth <= 32, "Hard coded shader value")

    const auto& nodes = mAccelerationStructure.GetNodes();
    BELL_ASSERT(!nodes.empty(), "BVH build error")
    const auto& indicies = mAccelerationStructure.GetIndices();
    BELL_ASSERT(!indicies.empty(), "BVH build error")
    mNodesBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, nodes.size() * sizeof(nanort::BVHNode<float>),
                                            nodes.size() * sizeof(nanort::BVHNode<float>), "BVH nodes");
    mIndiciesBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, indicies.size() * sizeof(uint32_t),
                                               indicies.size() * sizeof(uint32_t), "BVH indicies");
    mPrimToMatIDBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, mPrimitiveMaterialID.size() * sizeof(MaterialInfo),
                                                  mPrimitiveMaterialID.size() * sizeof(MaterialInfo), "PrimToMatID");
    mPositionBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, mPositions.size() * sizeof(float3),
                                               mPositions.size() * sizeof(float3), "Static positions");

    mUVBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, mUVs.size() * sizeof(float2),
                                               mUVs.size() * sizeof(float2), "Static uv");

    mPositionIndexBuffer = std::make_unique<Buffer>(eng->getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, mIndexBuffer.size() * sizeof(uint32_t),
                                               mIndexBuffer.size() * sizeof(uint32_t), "Static positions indicies");

    (*mNodesBuffer)->setContents(nodes.data(), nodes.size() * sizeof(nanort::BVHNode<float>));
    (*mIndiciesBuffer)->setContents(indicies.data(), indicies.size() * sizeof(uint32_t));
    (*mPrimToMatIDBuffer)->setContents(mPrimitiveMaterialID.data(), mPrimitiveMaterialID.size() * sizeof(MaterialInfo));
    (*mPositionBuffer)->setContents(mPositions.data(), mPositions.size() * sizeof(float3));
    (*mUVBuffer)->setContents(mUVs.data(), mUVs.size() * sizeof(float2));
    (*mPositionIndexBuffer)->setContents(mIndexBuffer.data(), mIndexBuffer.size() * sizeof(uint32_t));

    mBVH_SRS->addDataBufferRO(*mNodesBuffer);
    mBVH_SRS->addDataBufferRO(*mIndiciesBuffer);
    mBVH_SRS->addDataBufferRO(*mPrimToMatIDBuffer);
    mBVH_SRS->addDataBufferRO(*mPositionBuffer);
    mBVH_SRS->addDataBufferRO(*mUVBuffer);
    mBVH_SRS->addDataBufferRO(*mPositionIndexBuffer);
    mBVH_SRS->finalise();

}

void RayTracingScene::renderSceneToMemory(const Camera& camera, const uint32_t x, const uint32_t y, uint8_t* memory, ThreadPool& threadPool) const
{
    const float3 forward = camera.getDirection();
    const float3 up = camera.getUp();
    const float3 right = camera.getRight();

    const float3 origin = camera.getPosition();
    const float farPlane = camera.getFarPlane();
    const float aspect = camera.getAspect();

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
        ray.max_t = farPlane;
        //ray.type = nanort::RAY_TYPE_PRIMARY;

        InterpolatedVertex frag;
        const bool hit = traceRay(ray, &frag);
        if(hit)
        {
            return shadePoint(frag, float4(origin, 1.0f), 10, 5);
        }
        else
        {
            return mScene->getCPUSkybox()->sampleCube4(dir);
        }
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

    const uint32_t processor_count = threadPool.getWorkerCount(); // use this many threads for tracing rays.
    std::vector<std::future<void>> handles{};
    for(uint32_t i = 1; i < processor_count; ++i)
    {
        handles.push_back(threadPool.addTask(trace_rays, i, processor_count));
    }

    trace_rays(0, processor_count);

    for(auto& thread : handles)
        thread.wait();
}


void RayTracingScene::renderSceneToFile(const Camera& camera, const uint32_t x, const uint32_t y, const char* path, ThreadPool& threadPool) const
{
    uint8_t* memory = new uint8_t[x * y * 4];
    BELL_ASSERT(memory, "Unable to allocate memory")

    renderSceneToMemory(camera, x, y, memory, threadPool);

    stbi_write_jpg(path, x, y, 4, memory, 100);

    delete[] memory;
}


RayTracingScene::InterpolatedVertex RayTracingScene::interpolateFragment(const uint32_t primID, const float u, const float v) const
{
    BELL_ASSERT((u + v) <= 1.1f, "Out of bounds")

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
    frag.mNormal = glm::normalize(float3(((1.0f - v - u) * firstNormal) + (u * secondNormal) + (v * thirdNormal)));
    frag.mVertexColour = ((1.0f - v - u) * firstColour) + (u * secondColour) + (v * thirdColour);
    frag.mPrimID = primID;

    return frag;
}


bool RayTracingScene::traceRay(const nanort::Ray<float>& ray, InterpolatedVertex *result) const
{
    nanort::TriangleIntersector triangle_intersecter(reinterpret_cast<const float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));
    nanort::TriangleIntersection intersection;
    bool hit = mAccelerationStructure.Traverse(ray, triangle_intersecter, &intersection);

    if(hit) // check for alpha tested geometry.
    {
        *result= interpolateFragment(intersection.prim_id, intersection.u, intersection.v);

        const std::vector<CPUImage>& materials = mScene->getCPUImageMaterials();
        BELL_ASSERT(result->mPrimID < mPrimitiveMaterialID.size(), "index out of bounds")
        const MaterialInfo& matInfo = mPrimitiveMaterialID[result->mPrimID];

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


bool RayTracingScene::traceRayNonAlphaTested(const nanort::Ray<float> &ray, InterpolatedVertex *result) const
{
    nanort::TriangleIntersector triangle_intersecter(reinterpret_cast<const float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));
    nanort::TriangleIntersection intersection;
    const bool hit = mAccelerationStructure.Traverse(ray, triangle_intersecter, &intersection);

    if(hit)
    {
        *result = interpolateFragment(intersection.prim_id, intersection.u, intersection.v);
    }

    return hit;
}


bool RayTracingScene::intersectsMesh(const nanort::Ray<float>& ray, uint64_t *instanceID)
{
    BELL_ASSERT(instanceID, "Need to supply valid pointer for mesh selection test")
    InterpolatedVertex vertex;
    if(traceRay(ray, &vertex))
    {
        const MaterialInfo& matInfo = mPrimitiveMaterialID[vertex.mPrimID];
        *instanceID = matInfo.instanceID;

        return true;
    }

    return false;
}


bool RayTracingScene::traceShadowRay(const InterpolatedVertex& position) const
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


float4 RayTracingScene::traceDiffuseRays(const InterpolatedVertex& frag, const float4& origin, const uint32_t sampleCount, const uint32_t depth) const
{
    // interpolate uvs.
    BELL_ASSERT(frag.mPrimID < mPrimitiveMaterialID.size(), "index out of bounds")
    const MaterialInfo& matInfo = mPrimitiveMaterialID[frag.mPrimID];
    Material mat = calculateMaterial(frag, matInfo);
    float4 diffuse = mat.diffuse;
    const float3 V = glm::normalize(float3(origin - frag.mPosition));

    DiffuseSampler sampler(sampleCount * depth);

    float4 result = float4{0.0f, 0.0f, 0.0f, 0.0f};
    float weight = 0.0f;
    for(uint32_t i = 0; i < sampleCount; ++i)
    {
        Sample sample = sampler.generateSample(frag.mNormal);
        weight += sample.P;

        const float NdotV = glm::clamp(glm::dot(float3(mat.normal), V), 0.0f, 1.0f);
        const float NdotL = glm::clamp(glm::dot(float3(mat.normal), sample.L), 0.0f, 1.0f);
        const float3 H = glm::normalize(V + sample.L);
        const float LdotH  = glm::clamp(glm::dot(sample.L, H), 0.0f, 1.0f);

        const float diffuseFactor = disneyDiffuse(NdotV, NdotL, LdotH, mat.specularRoughness.w);

        nanort::Ray<float> newRay{};
        newRay.org[0] = frag.mPosition.x;
        newRay.org[1] = frag.mPosition.y;
        newRay.org[2] = frag.mPosition.z;
        newRay.dir[0] = sample.L.x;
        newRay.dir[1] = sample.L.y;
        newRay.dir[2] = sample.L.z;
        newRay.min_t = 0.01f;
        newRay.max_t = 2000.0f;

        InterpolatedVertex intersection;
        const bool hit = traceRay(newRay, &intersection);
        if(hit)
        {
            result += sample.P * diffuseFactor * shadePoint(intersection, frag.mPosition, sampleCount, depth - 1);
        }
        else
        {
            result += sample.P * diffuseFactor * mScene->getCPUSkybox()->sampleCube4(sample.L); // miss so sample skybox.
        }
    }

    diffuse *= result / weight;

    return diffuse;// + float4(mat.emissiveOcclusion.x, mat.emissiveOcclusion.y, mat.emissiveOcclusion.z, 1.0f);
}


float4 RayTracingScene::traceSpecularRays(const InterpolatedVertex& frag, const float4 &origin, const uint32_t sampleCount, const uint32_t depth) const
{
    // interpolate uvs.
    BELL_ASSERT(frag.mPrimID < mPrimitiveMaterialID.size(), "index out of bounds")
    const MaterialInfo& matInfo = mPrimitiveMaterialID[frag.mPrimID];
    Material mat = calculateMaterial(frag, matInfo);
    float4 specular = float4(mat.specularRoughness.x, mat.specularRoughness.y, mat.specularRoughness.z, 1.0f);

    SpecularSampler sampler(sampleCount * depth);

    const float3 V = glm::normalize(float3(origin - frag.mPosition));

    float4 result = float4{0.0f, 0.0f, 0.0f, 0.0f};
    float weight = 0.0f;
    for(uint32_t i = 0; i < sampleCount; ++i)
    {
        Sample sample = sampler.generateSample(frag.mNormal, V, mat.specularRoughness.w);
        weight += sample.P;
        BELL_ASSERT(sample.P >= 0.0f, "")

        nanort::Ray<float> newRay{};
        newRay.org[0] = frag.mPosition.x;
        newRay.org[1] = frag.mPosition.y;
        newRay.org[2] = frag.mPosition.z;
        newRay.dir[0] = sample.L.x;
        newRay.dir[1] = sample.L.y;
        newRay.dir[2] = sample.L.z;
        newRay.min_t = 0.01f;
        newRay.max_t = 2000.0f;

        const float specularFactor = specular_GGX(mat.normal, V, sample.L, mat.specularRoughness.w, float3(mat.specularRoughness.x, mat.specularRoughness.y, mat.specularRoughness.z));
        BELL_ASSERT(specularFactor >= 0.0f, "")

        InterpolatedVertex intersection;
        const bool hit = traceRay(newRay, &intersection);
        if(hit)
        {
            result += specularFactor * sample.P * shadePoint(intersection, frag.mPosition, sampleCount, depth - 1);
        }
        else
        {
            result += specularFactor * sample.P * mScene->getCPUSkybox()->sampleCube4(sample.L); // miss so sample skybox.
        }
    }

    specular *= result / weight;

    //BELL_ASSERT(!glm::all(glm::isnan(specular)), "invalid value")
    return glm::all(glm::isnan(specular)) ? float4(0.0f, 0.0f, 0.0f, 1.0f) : specular;// + float4(mat.emissiveOcclusion.x, mat.emissiveOcclusion.y, mat.emissiveOcclusion.z, 1.0f);
}


RayTracingScene::Material RayTracingScene::calculateMaterial(const InterpolatedVertex& frag, const MaterialInfo& info) const
{
    Material mat;
    mat.diffuse = frag.mVertexColour;
    mat.normal = glm::normalize(frag.mNormal);
    mat.specularRoughness = float4(0.0f, 0.0f, 0.0f, 1.0f);
    mat.emissiveOcclusion = float4(0.0f, 0.0f, 0.0f, 1.0);

    auto fresnelSchlickRoughness = [](const float cosTheta, const float3 F0, const float roughness)
    {
        return F0 + (glm::max(float3(1.0f - roughness), F0) - F0) * std::pow(1.0f - cosTheta, 5.0f);
    };

    uint32_t nextMaterialSlot = 0;
    const auto& materials = mScene->getCPUImageMaterials();

    if(info.materialFlags & MaterialType::Diffuse)
    {
        BELL_ASSERT(info.materialIndex < materials.size(), "material index out of bounds")
        mat.diffuse = materials[info.materialIndex].sample4(frag.mUV);
        ++nextMaterialSlot;
    }
    else if(info.materialFlags & MaterialType::Albedo)
        ++nextMaterialSlot;

    if(info.materialFlags & MaterialType::Normals)
    {
        ++nextMaterialSlot; // TODO work out how to do this without derivitives!!.
    }

    if(info.materialFlags & MaterialType::Roughness)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        mat.specularRoughness.w = materials[info.materialIndex + nextMaterialSlot].sample(frag.mUV);
        ++nextMaterialSlot;
    }
    else if(info.materialFlags & MaterialType::Gloss)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        mat.specularRoughness.w = 1.0f - materials[info.materialIndex + nextMaterialSlot].sample(frag.mUV);
        ++nextMaterialSlot;
    }

    float metalness = 0.0f;
    if(info.materialFlags & MaterialType::Specular)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        const float3 spec = materials[info.materialIndex + nextMaterialSlot].sample4(frag.mUV);
        mat.specularRoughness.x = spec.x;
        mat.specularRoughness.y = spec.y;
        mat.specularRoughness.z = spec.z;
        ++nextMaterialSlot;
    }
    else if(info.materialFlags & MaterialType::CombinedSpecularGloss)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        mat.specularRoughness = materials[info.materialIndex + nextMaterialSlot].sample4(frag.mUV);
        mat.specularRoughness.w = 1.0f - mat.specularRoughness.w;
        ++nextMaterialSlot;
    }
    else if(info.materialFlags & MaterialType::Metalness)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        metalness = materials[info.materialIndex + nextMaterialSlot].sample(frag.mUV);
        ++nextMaterialSlot;
    }
    else if(info.materialFlags & MaterialType::CombinedMetalnessRoughness)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        const float4 metalnessRoughness = materials[info.materialIndex + nextMaterialSlot].sample4(frag.mUV);
        metalness = metalnessRoughness.z;
        mat.specularRoughness.w = metalnessRoughness.y;
        ++nextMaterialSlot;
    }

    if(info.materialFlags & MaterialType::Albedo)
    {
        BELL_ASSERT(info.materialIndex < materials.size(), "material index out of bounds")
        const float4 albedo = materials[info.materialIndex].sample4(frag.mUV);
        mat.diffuse = albedo * (1.0f - 0.04f) * (1.0f - metalness);
        mat.diffuse.w = albedo.w;// Preserve the alpha chanle.

        const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), float3(albedo), metalness);
        mat.specularRoughness.x = F0.x;
        mat.specularRoughness.y = F0.y;
        mat.specularRoughness.z = F0.z;
    }

    if(info.materialFlags & MaterialType::Emisive)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        const float3 emissive = materials[info.materialIndex + nextMaterialSlot].sample4(frag.mUV);
        mat.emissiveOcclusion.x = emissive.x;
        mat.emissiveOcclusion.y = emissive.y;
        mat.emissiveOcclusion.z = emissive.z;
        ++nextMaterialSlot;
    }

    if(info.materialFlags & MaterialType::AmbientOcclusion)
    {
        BELL_ASSERT((info.materialIndex + nextMaterialSlot) < materials.size(), "material index out of bounds")
        mat.emissiveOcclusion.w = materials[info.materialIndex + nextMaterialSlot].sample(frag.mUV);
    }

    return mat;
}


bool RayTracingScene::isVisibleFrom(const float3& dst, const float3& src) const
{
    const float3 direction = dst - src;
    const float3 normalizedDir = glm::normalize(direction);

    nanort::Ray ray{};
    ray.dir[0] = normalizedDir.x;
    ray.dir[1] = normalizedDir.y;
    ray.dir[2] = normalizedDir.z;
    ray.org[0] = src.x;
    ray.org[1] = src.y;
    ray.org[2] = src.z;
    ray.min_t = 0.001f;
    ray.max_t = 200.0f;

    InterpolatedVertex frag;
    const bool hit = traceRay(ray, &frag);
    if(hit)
    {
        const float dist = glm::length(direction);
        const float distToHit = glm::length(float3(frag.mPosition) - src);

        return distToHit >= dist;
    }

    return true;
}


float4 RayTracingScene::shadePoint(const InterpolatedVertex& frag, const float4 &origin, const uint32_t sampleCount, const uint32_t depth) const
{
    if(depth == 0)
        return float4(0.0f, 0.0f, 0.0f, 1.0f);

    const bool shadowed = traceShadowRay(frag);

    const float4 diffuse = traceDiffuseRays(frag, origin, sampleCount, depth);
    const float4 specular = traceSpecularRays(frag, origin, sampleCount, depth);

    return diffuse + specular;// * (shadowed ? 0.15f : 1.0f);
}


void RayTracingScene::updateCPUAccelerationStructure(const Scene* scene)
{
    mScene = scene;
    uint64_t vertexOffset = 0;
    InstanceID instanceID = 0;
    for (const auto& [id, entry] : scene->getInstanceMap())
    {
        const MeshInstance* instance = scene->getMeshInstance(id);
        const StaticMesh* mesh = instance->getMesh();
        const uint32_t vertexStride = mesh->getVertexStride();

        // add Index data.
        const auto& indexBuffer = mesh->getIndexData();
        std::transform(indexBuffer.begin(), indexBuffer.end(), std::back_inserter(mIndexBuffer), [vertexOffset](const uint32_t index)
        {
            return vertexOffset + index;
        });

        // add material mappings
        for (uint32_t i = 0; i < indexBuffer.size() / 3; ++i)
        {
            mPrimitiveMaterialID.push_back({ instanceID, instance->getMaterialIndex(), instance->getMaterialFlags() });
        }

        // Transform and add vertex data.
        const auto& vertexData = mesh->getVertexData();
        for (uint32_t i = 0; i < vertexData.size(); i += vertexStride)
        {
            const unsigned char* vert = &vertexData[i];
            const float* positionPtr = reinterpret_cast<const float*>(vert);

            const float4 position = float4{ positionPtr[0], positionPtr[1], positionPtr[2], positionPtr[3] };
            BELL_ASSERT(position.w == 1.0f, "Probably incorrect pointer")
            const float4 transformedPosition = instance->getTransMatrix() * position;
            BELL_ASSERT(transformedPosition.w == 1.0f, "Non afine transform")

           mPositions.emplace_back(transformedPosition.x, transformedPosition.y, transformedPosition.z);

            const float2 uv = float2{ positionPtr[4], positionPtr[5] };
            mUVs.push_back(uv);

            const float4 normal = unpackNormal(*reinterpret_cast<const uint32_t*>(&positionPtr[6]));
            mNormals.push_back(normal);

            const float4 colour = unpackColour(*reinterpret_cast<const uint32_t*>(&positionPtr[7]));
            mVertexColours.push_back(colour);
        }

        vertexOffset += mesh->getVertexCount();
        ++instanceID;
    }

    mMeshes = std::make_unique<nanort::TriangleMesh<float>>(reinterpret_cast<float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));
    mPred = std::make_unique<nanort::TriangleSAHPred<float>>(reinterpret_cast<float*>(mPositions.data()), mIndexBuffer.data(), sizeof(float3));

    BELL_ASSERT((mIndexBuffer.size() % 3) == 0, "Invalid index buffer size")
    bool success = mAccelerationStructure.Build(mIndexBuffer.size() / 3, *mMeshes, *mPred);
    BELL_ASSERT(success, "Failed to build BVH")
}