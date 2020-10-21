#ifndef SCENE_HPP
#define SCENE_HPP

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"

#include "Engine/OctTree.hpp"
#include "Engine/Camera.hpp"
#include "Engine/StaticMesh.h"
#include "Engine/Animation.hpp"
#include "Engine/CPUImage.hpp"
#include "Engine/Instance.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Engine;


using SceneID = uint64_t;
using InstanceID = uint64_t;
constexpr InstanceID kInvalidInstanceID = std::numeric_limits<InstanceID>::max();

enum class MeshType
{
	Dynamic,
	Static
};

enum class InstanceType
{
    StaticMesh,
    DynamicMesh,
    Light
};

enum class LightType : uint32_t
{
	Point = 0,
	Spot = 1,
	Area = 2,
	Strip = 3
};

enum class MaterialType
{
    Albedo = 1,
    Diffuse = 1 << 1,
    Normals = 1 << 2,
    Roughness = 1 << 3,
    Gloss = 1 << 4,
    Metalness = 1 << 5,
    Specular = 1 << 6,
    CombinedMetalnessRoughness = 1 << 7,
    AmbientOcclusion = 1 << 8,
    Emisive = 1 << 9,
    CombinedSpecularGloss = 1 << 10,

    AlphaTested = 1 << 11,
    Transparent = 1 << 12
};
inline uint32_t operator&(const uint32_t lhs, const MaterialType rhs)
{
    return lhs & static_cast<uint32_t>(rhs);
}
inline uint32_t operator|(const MaterialType lhs, const MaterialType rhs)
{
    return static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs);
}
inline uint32_t operator|(const uint32_t lhs, const MaterialType rhs)
{
    return lhs | static_cast<uint32_t>(rhs);
}

enum class PBRType
{
    Metalness,
    Specular
};

enum InstanceFlags
{
    Draw = 1,
    DrawAABB = 1 << 1,
    DrawWireFrame = 1 << 2,
    DrawGuizmo = 1 << 3
};

class Scene;

class MeshInstance : public Instance
{
public:
    MeshInstance(Scene* scene,
                 SceneID mesh,
                 const float4x3& trans,
                 const uint32_t materialID,
                 const uint32_t materialFLags,
                 const std::string& name = "") :
        Instance(trans, name),
        mScene(scene),
        mMesh(mesh),
        mMaterialIndex{materialID},
        mMaterialFlags{materialFLags},
        mInstanceFlags{InstanceFlags::Draw} {}

    StaticMesh* getMesh();
    const StaticMesh* getMesh() const;

    uint32_t getMaterialIndex() const
    {
        return mMaterialIndex;
    }

    void setMaterialIndex(const uint32_t i)
    {
        mMaterialIndex = i;
    }

    uint32_t getInstanceFlags() const
    {
        return mInstanceFlags;
    }

    void setInstanceFlags(const uint32_t flags)
    {
        mInstanceFlags = flags;
    }

    uint32_t getMaterialFlags() const
    {
        return mMaterialFlags;
    }

    void setMaterialFlags(const uint32_t flags)
    {
        mMaterialFlags = flags;
    }

    MeshEntry getMeshShaderEntry() const
    {
        MeshEntry entry{};
        entry.mTransformation = transpose(float4x3(getTransMatrix()));
        entry.mPreviousTransformation = transpose(float4x3(getPreviousTransMatrix()));
        entry.mMaterialIndex = mMaterialIndex;
        entry.mMaterialFlags = mMaterialFlags;

        return entry;
    }

private:

    Scene* mScene;
    SceneID mMesh;
    uint32_t mMaterialIndex;
    uint32_t mMaterialFlags;
    uint32_t mInstanceFlags;
};


class Scene
{
public:

    struct ShadowingLight;

    Scene(const std::filesystem::path& path);
    ~Scene();

    void setPath(const std::filesystem::path& path)
    {
        mPath = path;
    }

    const std::filesystem::path& getPath() const
    {
        return mPath;
    }

    std::vector<InstanceID> loadFromFile(const int vertAttributes, Engine*);
	void loadSkybox(const std::array<std::string, 6>& path, Engine*);

    SceneID       addMesh(const StaticMesh&, MeshType);
    InstanceID    addMeshInstance(const SceneID,
                                  const InstanceID parentInstance,
                                  const float4x4&,
                                  const uint32_t materialIndex,
                                  const uint32_t materialFlags,
                                  const std::string& name = "");
    void          removeMeshInstance(const InstanceID);

    void          uploadData(Engine*);
    void          computeBounds(const MeshType);

    std::vector<const MeshInstance*> getViewableMeshes(const Frustum&) const;

    const std::vector<MeshInstance>& getStaticMeshInstances() const
    {
        return mStaticMeshInstances;
    }

    const std::vector<MeshInstance>& getDynamicMeshInstances() const
    {
        return mDynamicMeshInstances;
    }

    Frustum getShadowingLightFrustum() const;

    MeshInstance*       getMeshInstance(const InstanceID);
    StaticMesh*         getMesh(const SceneID);
    const StaticMesh*   getMesh(const SceneID) const;
	const std::unique_ptr<ImageView>& getSkybox() const
	{
		return mSkyboxView;
	}
	
    const std::unique_ptr<CPUImage>& getCPUSkybox() const
    {
        return mCPUSkybox;
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

    void setShadowingLight(const Camera&);

    Camera& getShadowLightCamera()
    {
        return mShadowLightCamera;
    }

    const ShadowingLight& getShadowingLight() const
    {
        return mShadowingLight;
    }

    ShadowingLight& getShadowingLight()
    {
        return mShadowingLight;
    }

	const std::vector<ImageView>& getMaterials() const
	{
		return mMaterialImageViews;
	}

    std::vector<ImageView>& getMaterials()
    {
        return mMaterialImageViews;
    }

	struct Material
	{
        std::string mName;
        Image* mAlbedoorDiffuse;
        Image* mNormals;
        Image* mRoughnessOrGloss;
        Image* mMetalnessOrSpecular;
        Image* mEmissive;
        Image* mAmbientOcclusion;
        uint32_t mMaterialTypes;
        uint32_t mMaterialOffset;

        void updateLastAccessed()
        {
            if(mAlbedoorDiffuse)
                (*mAlbedoorDiffuse)->updateLastAccessed();
            if(mNormals)
                (*mNormals)->updateLastAccessed();
            if(mRoughnessOrGloss)
                (*mRoughnessOrGloss)->updateLastAccessed();
            if(mMetalnessOrSpecular)
                (*mMetalnessOrSpecular)->updateLastAccessed();
            if(mEmissive)
                (*mEmissive)->updateLastAccessed();
            if(mAmbientOcclusion)
                (*mAmbientOcclusion)->updateLastAccessed();
        }
	};

    const std::vector<Material>& getMaterialsBase() const
    {
        return mMaterials;
    }

    struct MaterialPaths
    {
        std::string mName;
        std::string mAlbedoorDiffusePath;
        std::string mNormalsPath;
        std::string mRoughnessOrGlossPath;
        std::string mMetalnessOrSpecularPath;
        std::string mEmissivePath;
        std::string mAmbientOcclusionPath;
        uint32_t mMaterialTypes;
        uint32_t mMaterialOffset;
    };

    const std::vector<Material>& getMaterialDescriptions() const
    {
        return mMaterials;
    }

    const std::vector<CPUImage>& getCPUImageMaterials() const
    {
        return mCPUMaterials;
    }

    void addMaterial(const Material& mat);
    void addMaterial(const MaterialPaths& mat, Engine *eng);

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

    struct ShadowCascades
    {
        float mNearEnd;
        float mMidEnd;
        float mFarEnd;
    };

    void setShadowCascades(const float neadEnd, const float midEnd, const float farEnd)
    {
        mCascadesInfo.mNearEnd = neadEnd;
        mCascadesInfo.mMidEnd = midEnd;
        mCascadesInfo.mFarEnd = farEnd;
    }

    const ShadowCascades& getShadowCascades() const
    {
        return mCascadesInfo;
    }

    ShadowCascades& getShadowCascades()
    {
        return mCascadesInfo;
    }

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

    void updateMaterialsLastAccess()
    {
        for(auto& mat : mMaterials)
            mat.updateLastAccessed();
    }

    SkeletalAnimation& getSkeletalAnimation(const InstanceID id, const std::string& name)
    {
        MeshInstance* inst = getMeshInstance(id);
        BELL_ASSERT(inst->getMesh()->hasAnimations(), "Requesting animation from mesh without animations")
        return inst->getMesh()->getSkeletalAnimation(name);
    }

    BlendMeshAnimation& getBlendMeshAnimation(const InstanceID id, const std::string& name)
    {
        MeshInstance* inst = getMeshInstance(id);
        BELL_ASSERT(inst->getMesh()->hasAnimations(), "Requesting animation from mesh without animations")
            return inst->getMesh()->getBlendMeshAnimation(name);
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

	using MaterialMappings = std::map<aiString, uint32_t, AiStringComparitor>;

    // return a mapping between mesh name and material index from the Bell material file format
	MaterialMappings loadMaterialsInternal(Engine*);
    // Loads materials at the index specified by the external scene file.
    void loadMaterialsExternal(Engine*, const aiScene *scene);

    void parseNode(const aiScene* scene,
                   const aiNode* node,
                   const aiMatrix4x4& parentTransofrmation,
                   const InstanceID parentID,
                   const int vertAttributes,
                   const MaterialMappings& materialIndexMappings,
                   std::unordered_map<const aiMesh *, SceneID> &meshMappings,
                   std::vector<InstanceID>& instanceIds);

	void addLights(const aiScene* scene);

    std::filesystem::path mPath;

    std::vector<std::pair<StaticMesh, MeshType>> mSceneMeshes;

    std::vector<MeshInstance> mStaticMeshInstances;
    std::vector<uint32_t>     mFreeStaticMeshIndicies;
    std::vector<MeshInstance> mDynamicMeshInstances;
    std::vector<uint32_t>     mFreeDynamicMeshIndicies;

    OctTree<MeshInstance*> mStaticMeshBoundingVolume;
    OctTree<MeshInstance*> mDynamicMeshBoundingVolume;

    AABB mSceneAABB;

	Camera mSceneCamera;

	std::vector<Material> mMaterials;
	std::vector<ImageView> mMaterialImageViews;

    std::vector<CPUImage> mCPUMaterials;

    std::vector<Light> mLights;
    Camera mShadowLightCamera;
    ShadowingLight mShadowingLight;
    ShadowCascades mCascadesInfo;

    struct InstanceInfo
    {
        InstanceType mtype;
        uint64_t mIndex;
    };
    uint64_t mNextInstanceID;
    std::unordered_map<InstanceID, InstanceInfo> mInstanceMap;

	std::unique_ptr<Image> mSkybox;
	std::unique_ptr<ImageView> mSkyboxView;
    std::unique_ptr<CPUImage> mCPUSkybox;
};

#endif
