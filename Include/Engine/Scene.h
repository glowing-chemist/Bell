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

    void          uploadData(Engine*);
    void          computeBounds(const MeshType);

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
        float mIntensity;
		float mRadius;
		LightType mType;
        float mPadding;
	};
    size_t addLight(const Light& light)
    {
        const size_t lightID = mLights.size();
        mLights.push_back(light);

        return lightID;
    }

    Light& getLight(const size_t ID)
    {
        BELL_ASSERT(ID < mLights.size(), "Invalid light ID")
        return mLights[ID];
    }

    void clearLights()
    { mLights.clear(); }

    const std::vector<Light>& getLights() const
    { return mLights; }

    struct ShadowingLight
    {
        glm::mat4 mViewMatrix;
        glm::mat4 mInverseView;
        glm::mat4 mViewProj;
        glm::vec4 mPosition;
        glm::vec4 mDirection;
    };

	void loadMaterials(Engine*);

	struct Intersection
	{
		Intersection(const MeshInstance* m, const MeshInstance* m2) :
			mEntry1{m},
			mEntry2{m2} {}

		const MeshInstance* mEntry1;
		const MeshInstance* mEntry2;
	};
	std::vector<Intersection> getIntersections() const;
	std::vector<Intersection> getIntersections(const InstanceID);
	std::vector<Intersection> getIntersections(const InstanceID IgnoreID, const AABB& aabbToTest);

	const AABB& getBounds() const
	{
		return mSceneAABB;
	}

private:

    void generateSceneAABB(const bool includeStatic);

	struct AiStringComparitor
	{
		bool operator()(const aiString& l, const aiString& r) const noexcept
		{
			return memcmp(l.C_Str(), r.C_Str(), std::min(l.length, r.length)) < 0;
		}
	};

	struct MeshInfo
	{
		uint32_t index;
		uint32_t attributes;
	};

	using MaterialMappings = std::map<aiString, MeshInfo, AiStringComparitor>;

	// return a mapping between mesh name and material index
	MaterialMappings loadMaterialsInternal(Engine*);

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

	std::vector<Material> mMaterials;
	std::vector<ImageView> mMaterialImageViews;

    std::vector<Light> mLights;
    ShadowingLight mShadowingLight;

	std::unique_ptr<Image> mSkybox;
	std::unique_ptr<ImageView> mSkyboxView;
};

#endif
