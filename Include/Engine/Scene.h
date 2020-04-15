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
#include <filesystem>
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
	Area = 2,
	Strip = 3
};


struct MeshInstance
{
    MeshInstance(StaticMesh* mesh, const float4x3& trans, const uint32_t materialID) :
        mMesh(mesh),
        mTransformation(trans),
        mPreviousTransformation(trans),
        mMaterialID{materialID} {}

    StaticMesh* mMesh;

    const float4x4& getTransMatrix() const
    {
        return mTransformation;
    }

    uint32_t getmaterialID() const
    {
        return mMaterialID;
    }

    void setTransMatrix(const float4x4& newTrans)
    {
        mPreviousTransformation = mTransformation;
        mTransformation = newTrans;
    }

    MeshEntry getMeshShaderEntry() const
    {
        MeshEntry entry{};
        entry.mTransformation = float3x4(mTransformation);
        entry.mPreviousTransformation = float3x4(mPreviousTransformation);
        entry.mMaterialIndex = mMaterialID;
        entry.mAttributes = mMesh->getAttributes();

        return entry;
    }

private:
    float4x4 mTransformation;
    float4x4 mPreviousTransformation;
    uint32_t mMaterialID;
};


class Scene
{
public:

    struct ShadowingLight;

	Scene(const std::string& name);
    Scene(Scene&&);
    Scene& operator=(Scene&&);

	void loadFromFile(const int vertAttributes, Engine*);
	void loadSkybox(const std::array<std::string, 6>& path, Engine*);

    SceneID       addMesh(const StaticMesh&, MeshType);
    InstanceID    addMeshInstance(const SceneID, const float4x3&, const uint32_t materialID);

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

    void setShadowingLight(const float3& position, const float3& direction, const float3& up);

    const ShadowingLight& getShadowingLight() const
    {
        return mShadowingLight;
    }

	const std::vector<ImageView>& getMaterials() const
	{
		return mMaterialImageViews;
	}

	struct Material
	{
		Image mAlbedo;
		Image mNormals;
		Image mRoughness;
		Image mMetalness;
	};

	struct Light
	{
        // Helpers to create the different light varieties.
        static Light pointLight(const float4& position, const float4& albedo, const float intensity, const float radius);
        static Light spotLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float angle);
        static Light areaLight(const float4& position, const float4& direction, const float4& up, const float4& albedo, const float intensity, const float radius, const float size);
        static Light stripLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float size);

        float4 mPosition;
        float4 mDirection;
        float4 mUp; // Only relevant for area and line.
        float4 mAlbedo;
        float mIntensity;
		float mRadius;
		LightType mType;
        float mAngleSize; // Cone angle for spot, side lenght for area and length for strip.
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
        float4x4 mViewMatrix;
        float4x4 mInverseView;
        float4x4 mViewProj;
        float4    mPosition;
        float4    mDirection;
        float4    mUp;
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
