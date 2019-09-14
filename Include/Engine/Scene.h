#ifndef SCENE_HPP
#define SCENE_HPP

#include "Engine/BVH.hpp"
#include "Engine/Camera.hpp"
#include "Engine/StaticMesh.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>


using SceneID = uint64_t;
using InstanceID = int64_t;

enum class MeshType
{
	Dynamic,
	Static
};


class Scene
{
public:

    struct MeshInstance;

	Scene(const std::string& name);
    Scene(Scene&&);
    Scene& operator=(Scene&&);

    void loadFromFile(const int vertAttributes);

    SceneID       addMesh(const StaticMesh&, MeshType);
    InstanceID    addMeshInstance(const SceneID, const glm::mat4&);

    void          finalise();

    std::vector<MeshInstance*> getViewableMeshes() const;

    MeshInstance* getMeshInstance(const InstanceID);
	
    void setCamera(const Camera& camera)
	{
		mSceneCamera = camera;
	}
	
	Camera& getCamera()
	{
		return mSceneCamera;
	}

    const Camera& getCamera() const
    {
        return mSceneCamera;
    }

    struct MeshInstance
    {
        StaticMesh* mMesh;
        glm::mat3 mTransformation;
    };

private:

    void generateSceneAABB(const bool includeStatic);

    void parseNode(const aiScene* scene,
                   const aiNode* node,
                   const aiMatrix4x4& parentTransofrmation,
                   const int vertAttributes);

    std::string mName;

    std::vector<std::pair<StaticMesh, MeshType>> mSceneMeshes;

    std::vector<MeshInstance> mStaticMeshInstances;
    std::vector<MeshInstance> mDynamicMeshInstances;

    BVH<MeshInstance*> mStaticMeshBoundingVolume;
    BVH<MeshInstance*> mDynamicMeshBoundingVolume;

    AABB mSceneAABB;

	Camera mSceneCamera;

    std::atomic_bool mFinalised;
};

#endif
