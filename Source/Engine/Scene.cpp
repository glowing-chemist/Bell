#include "Engine/Scene.h"
#include "Core/BellLogging.hpp"

#include <algorithm>
#include <iterator>


Scene::Scene(const std::string& name) :
    mName{name},
    mSceneMeshes(),
    mStaticMeshBoundingVolume(),
    mDynamicMeshBoundingVolume(),
    mSceneCamera(float3(), float3()),
    mFinalised(false)
{
}


SceneID Scene::addMesh(const StaticMesh& mesh, MeshType meshType)
{
    SceneID id = mSceneMeshes.size();

    mSceneMeshes.push_back({mesh, meshType});

    return id;
}


// It's invalid to use the InstanceID for a static mesh for anything other than state tracking.
// As the BVH for them will not be updated.
InstanceID Scene::addMeshInstace(const SceneID meshID, const glm::mat4& transformation)
{
    auto& [mesh, meshType] = mSceneMeshes[meshID];

    InstanceID id = 0;

    if(meshType == MeshType::Static)
    {
        id = static_cast<InstanceID>(mStaticMeshInstances.size() + 1);

        mStaticMeshInstances.push_back({&mesh, transformation});
    }
    else
    {
        id = -static_cast<InstanceID>(mDynamicMeshInstances.size() - 1);

        mDynamicMeshInstances.push_back({&mesh, transformation});
    }

    return id;
}


void Scene::finalise()
{
    const bool firstTime = !mFinalised.load(std::memory_order::memory_order_relaxed);

    mFinalised.store(true);

    generateSceneAABB(firstTime);

    if(firstTime)
    {
        //Build the static meshes BVH structure.
        std::vector<std::pair<AABB, MeshInstance*>> staticBVHMeshes{};

        std::transform(mStaticMeshInstances.begin(), mStaticMeshInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance& instance)
                        { return std::pair<AABB, MeshInstance*>{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

        BVHFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);

        mStaticMeshBoundingVolume = staticBVHFactory.generateBVH();
    }

    //Build the dynamic meshes BVH structure.
    std::vector<std::pair<AABB, MeshInstance*>> dynamicBVHMeshes{};

    std::transform(mDynamicMeshInstances.begin(), mDynamicMeshInstances.end(), std::back_inserter(dynamicBVHMeshes),
                   [](MeshInstance& instance)
                    { return std::pair<AABB, MeshInstance*>{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

    BVHFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

    mStaticMeshBoundingVolume = dynamicBVHFactory.generateBVH();
}


std::vector<Scene::MeshInstance*> Scene::getViewableMeshes() const
{
    std::vector<MeshInstance*> instances;

    Frustum currentFrustum = mSceneCamera.getFrustum();

    std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.containedWithin(currentFrustum, EstimationMode::Over);
    std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.containedWithin(currentFrustum, EstimationMode::Over);

    instances.insert(instances.end(), staticMeshes.begin(), staticMeshes.end());
    instances.insert(instances.end(), dynamicMeshes.begin(), dynamicMeshes.end());

    return instances;
}


Scene::MeshInstance* Scene::getMeshInstance(const InstanceID id)
{
    BELL_ASSERT(id < 0, "Trying to fetch a static mesh, altering this after scene creation is undefined")

    uint64_t meshIndex = -(id + 1);

    return &mDynamicMeshInstances[meshIndex];
}


// Find the smallest and largest coords amoung the meshes AABB and set the scene AABB to those.
void Scene::generateSceneAABB(const bool includeStatic)
{
    Cube sceneCube = mSceneAABB.getCube();

    if(includeStatic)
    {
        for(const auto& instance : mStaticMeshInstances)
        {
            AABB meshAABB = instance.mMesh->getAABB() * instance.mTransformation;
            Cube instanceCube = meshAABB.getCube();

            sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

            sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
        }
    }

    for(const auto& instance : mDynamicMeshInstances)
    {
        AABB meshAABB = instance.mMesh->getAABB() * instance.mTransformation;
        Cube instanceCube = meshAABB.getCube();

        sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

        sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
    }

    mSceneAABB = AABB(sceneCube);
}

