#ifndef SCENE_HPP
#define SCENE_HPP

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "Engine/OctTree.hpp"
#include "Engine/Camera.hpp"
#include "Engine/StaticMesh.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Engine;


using SceneID = uint64_t;
using InstanceID = int64_t;

enum class MeshType
{
	Dynamic,
	Static
};

enum class LightType : uint32_t
{
	Point = 0,
	Spot = 1,
	Area = 2
};


class Scene
{
public:

    struct MeshInstance;
    struct ShadowingLight;

	Scene(const std::string& name);
    Scene(Scene&&);
    Scene& operator=(Scene&&);

	void loadFromFile(const int vertAttributes, Engine*);
	void loadSkybox(const std::array<std::string, 6>& path, Engine*);

    SceneID       addMesh(const StaticMesh&, MeshType);
    InstanceID    addMeshInstance(const SceneID, const glm::mat4&);

    void          finalise();

    std::vector<const MeshInstance*> getViewableMeshes(const Frustum&) const;

    Frustum getShadowingLightFrustum() const;

    MeshInstance* getMeshInstance(const InstanceID);
	const std::unique_ptr<ImageView>& getSkybox() const
	{
		return mSkyboxView;
	}
	
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

    void setShadowingLight(const glm::vec3& position, const glm::vec3& direction);

    const ShadowingLight& getShadowingLight() const
    {
        return mShadowingLight;
    }

	const std::vector<ImageView>& getMaterials() const
	{
		return mMaterialImageViews;
	}

    struct MeshInstance
    {
        StaticMesh* mMesh;
		glm::mat4 mTransformation;
    };

	struct Material
	{
		Image mAlbedo;
		Image mNormals;
		Image mRoughness;
		Image mMetalness;
	};

	struct Light
	{
        float4 mPosition;
        float4 mDirection;
        float4 mALbedo;
        float mInfluence;
		LightType mType;
	};

    struct ShadowingLight
    {
        glm::mat4 mViewMatrix;
        glm::mat4 mInverseView;
        glm::mat4 mViewProj;
        glm::vec4 mPosition;
        glm::vec4 mDirection;
    };

    const std::vector<Light>& getLights() const
    { return mLights; }

private:

    void generateSceneAABB(const bool includeStatic);

	struct AiStringComparitor
	{
        bool operator()(const aiString& l, const aiString& r) const noexcept
		{
            return memcmp(l.C_Str(), r.C_Str(), std::min(l.length, r.length)) < 0;
		}
	};

    using MaterialMappings = std::map<aiString, uint32_t, AiStringComparitor>;

	// return a mapping between mesh name and material index
	MaterialMappings loadMaterials(Engine*);

	void parseNode(const aiScene* scene,
				   const aiNode* node,
				   const aiMatrix4x4& parentTransofrmation,
				   const int vertAttributes,
				   const MaterialMappings& materialIndexMappings);

	void addLights(const aiScene* scene);

    std::filesystem::path mName;

    std::vector<std::pair<StaticMesh, MeshType>> mSceneMeshes;

    std::vector<MeshInstance> mStaticMeshInstances;
    std::vector<MeshInstance> mDynamicMeshInstances;

    OctTree<MeshInstance*> mStaticMeshBoundingVolume;
    OctTree<MeshInstance*> mDynamicMeshBoundingVolume;

    AABB mSceneAABB;

	Camera mSceneCamera;

    std::atomic_bool mFinalised;

	std::vector<Material> mMaterials;
	std::vector<ImageView> mMaterialImageViews;

    std::vector<Light> mLights;
    ShadowingLight mShadowingLight;

	std::unique_ptr<Image> mSkybox;
	std::unique_ptr<ImageView> mSkyboxView;
};

#endif
