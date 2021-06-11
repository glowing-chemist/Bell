#ifndef SCENE_HPP
#define SCENE_HPP

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/AccelerationStructures.hpp"

#include "Engine/OctTree.hpp"
#include "Engine/Camera.hpp"
#include "Engine/StaticMesh.h"
#include "Engine/Animation.hpp"
#include "Engine/CPUImage.hpp"
#include "Engine/Instance.hpp"
#include "Engine/VoxelTerrain.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class RenderEngine;
class UberShaderStateCache;

using SceneID = uint64_t;
using InstanceID = uint64_t;
constexpr InstanceID kInvalidInstanceID = std::numeric_limits<InstanceID>::max();

enum class AccelerationStructure
{
    StaticMesh,
    DynamicMesh,
    Lights
};

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
    HeightMap = 1 << 11,

    AlphaTested = 1 << 20,
    Transparent = 1 << 21
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
                 InstanceID id,
                 const float4x3& trans,
                 const uint32_t materialID,
                 const uint32_t materialFLags,
                 const std::string& name = "");

    MeshInstance(Scene* scene,
                 SceneID mesh,
                 InstanceID id,
                 const float3& position,
                 const quat& rotation,
                 const float3& scale,
                 const uint32_t materialID,
                 const uint32_t materialFLags,
                 const std::string& name = "");

    StaticMesh* getMesh();
    const StaticMesh* getMesh() const;
    BottomLevelAccelerationStructure* getAccelerationStructure();
    const BottomLevelAccelerationStructure* getAccelerationStructure() const;

    SceneID getSceneID() const
    {
        return mMesh;
    }

    InstanceID  getID() const
    {
        return mID;
    }

    uint32_t getSubMeshCount() const
    {
	return mMaterials.size();
    }

    uint32_t getMaterialIndex(const uint32_t subMesh) const
    {
        return mMaterials[subMesh].mMaterialIndex;
    }

    void setMaterialIndex(const uint32_t subMesh, const uint32_t i)
    {
        mMaterials[subMesh].mMaterialIndex = i;
    }

    uint32_t getInstanceFlags() const
    {
        return mInstanceFlags;
    }

    void setInstanceFlags(const uint32_t flags)
    {
        mInstanceFlags = flags;
    }

    uint32_t getMaterialFlags(const uint32_t subMesh) const
    {
        return mMaterials[subMesh].mMaterialFlags;
    }

    void setMaterialFlags(const uint32_t subMesh, const uint32_t flags)
    {
        mMaterials[subMesh].mMaterialFlags = flags;
    }

    void draw(Executor*, UberShaderStateCache*, const uint32_t baseVertexOffset, const uint32_t baseIndexOffset) const;

private:

    MeshEntry getMeshShaderEntry(const uint32_t submesh_i, const SubMesh& submesh) const
    {
        MeshEntry entry{};
        entry.mTransformation = transpose(float4x3(getTransMatrix() * submesh.mTransform));
        entry.mPreviousTransformation = transpose(float4x3(getPreviousTransMatrix() * submesh.mTransform));
        entry.mMaterialIndex = mMaterials[submesh_i].mMaterialIndex;
        entry.mMaterialFlags = mMaterials[submesh_i].mMaterialFlags;

        return entry;
    }

    Scene* mScene;
    SceneID mMesh;
    InstanceID mID;
    struct MaterialEntry
    {
        uint32_t mMaterialIndex;
        uint32_t mMaterialFlags;
    };
    std::vector<MaterialEntry> mMaterials;
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

    std::vector<InstanceID> loadFromFile(const int vertAttributes, RenderEngine*);
	void loadSkybox(const std::array<std::string, 6>& path, RenderEngine*);

    SceneID       addMesh(RenderEngine*, const StaticMesh& mesh, MeshType);
    SceneID       loadFile(const std::string& path, MeshType, RenderEngine *eng, const bool loadMaterials = true);
    InstanceID    addMeshInstance(const SceneID,
                                  const InstanceID parentInstance,
                                  const float4x4&,
                                  const uint32_t materialIndex,
                                  const uint32_t materialFlags,
                                  const std::string& name = "");
    InstanceID    addMeshInstance(const SceneID,
                                  const InstanceID parentInstance,
                                  const float3& position,
                                  const float3& size,
                                  const quat& rotation,
                                  const uint32_t materialIndex,
                                  const uint32_t materialFlags,
                                  const std::string& name = "");
    void          removeInstance(const InstanceID);

    void          uploadData(RenderEngine*);
    void          computeBounds(const AccelerationStructure);

    struct Light
    {
        // Helpers to create the different light varieties.
        static Light pointLight(const float4& position, const float4& albedo, const float intensity, const float radius);
        static Light spotLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float angle);
        static Light areaLight(const float4& position, const float4& direction, const float4& up, const float4& albedo, const float intensity, const float radius, const float2 size);
        static Light stripLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float2 size);

        AABB getAABB() const;

        float3 mPosition;
        float mIntensity;
        float3 mDirection; // direction for spotlight.
        float mRadius;
        float3 mUp;
        LightType mType;
        float4 mAlbedo;
        float3 mAngleSize; // angle for spotlight and side length fo area.
        uint32_t _padding;
    };

    std::vector<const MeshInstance*> getVisibleMeshes(const Frustum&) const;
    std::vector<Scene::Light*> getVisibleLights(const Frustum&) const;

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
    const MeshInstance*       getMeshInstance(const InstanceID) const;
    float3 getInstancePosition(const InstanceID) const;
    void   setInstancePosition(const InstanceID, const float3&);
    void   translateInstance(const InstanceID, const float3&);
    StaticMesh*         getMesh(const SceneID);
    const StaticMesh*   getMesh(const SceneID) const;
    BottomLevelAccelerationStructure*         getAccelerationStructure(const SceneID);
    const BottomLevelAccelerationStructure*   getAccelerationStructure(const SceneID) const;
    const std::unique_ptr<ImageView>& getSkybox() const
    {
	return mSkyboxView;
    }
	
    const std::unique_ptr<CPUImage>& getCPUSkybox() const
    {
        return mCPUSkybox;
    }

    void setCamera(Camera* camera)
    {
	mSceneCamera = camera;
    }
	
    Camera& getCamera()
    {
	return *mSceneCamera;
    }

    const Camera& getCamera() const
    {
        return *mSceneCamera;
    }

    void setShadowingLight(Camera*);

    Camera& getShadowLightCamera()
    {
        return *mShadowLightCamera;
    }

    const ShadowingLight& getShadowingLight() const
    {
        return mShadowingLight;
    }

    void updateShadowingLight();

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
        Image* mHeightMap;
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

    struct MaterialPaths
    {
        std::string mName;
        std::string mAlbedoorDiffusePath;
        std::string mNormalsPath;
        std::string mRoughnessOrGlossPath;
        std::string mMetalnessOrSpecularPath;
        std::string mEmissivePath;
        std::string mAmbientOcclusionPath;
        std::string mHeightMapPath;
        uint32_t mMaterialTypes;
        uint32_t mMaterialOffset;
    };

    const std::vector<Material>& getMaterialDescriptions() const
    {
        return mMaterials;
    }

    std::vector<Material>& getMaterialDescriptions()
    {
        return mMaterials;
    }

    const std::vector<CPUImage>& getCPUImageMaterials() const
    {
        return mCPUMaterials;
    }

    void addMaterial(const Material& mat);
    void addMaterial(const MaterialPaths& mat, RenderEngine *eng);

    InstanceID addLight(const Light& light);

    Light& getLight(const InstanceID id);

    const std::vector<Light>& getLights() const
    {
        return mLights;
    }

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

	void loadMaterials(RenderEngine*);

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

    void setRootTransform(const float4x4& trans)
    {
        mRootTransform = trans;
    }

    const float4x4& getRootTransform() const
    {
	    return mRootTransform;
    }

    struct InstanceInfo
    {
        InstanceType mtype;
        uint64_t mIndex;
    };
    const std::unordered_map<InstanceID, InstanceInfo>& getInstanceMap() const
    {
        return mInstanceMap;
    }

    void setTerrainGridSize(const uint3& size, const float voxelSize)
    {
        mTerrain = std::make_unique<VoxelTerrain>(size, voxelSize);
    }

    void initialiseTerrainFromTexture(const std::string& path)
    {
        BELL_ASSERT(mTerrain, "Terrain hasn't been initialised")
        mTerrain->initialiseFromHeightMap(path);
    }

    void initialiseTerrainFromData(const std::vector<int8_t>& data)
    {
        BELL_ASSERT(mTerrain, "Terrain hasn't been initialised")
        mTerrain->initialiseFromData(data);
    }

    const std::unique_ptr<VoxelTerrain>& getVoxelTerrain() const
    {
        return mTerrain;
    }

    void testPrint() const
    {
        printf("Lua test hook\n");
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
	MaterialMappings loadMaterialsInternal(RenderEngine*);
    // Loads materials at the index specified by the external scene file.
    void loadMaterialsExternal(RenderEngine*, const aiScene *scene);

    void parseNode(RenderEngine* eng,
                   const aiScene* scene,
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
    std::vector<BottomLevelAccelerationStructure> mSceneAccelerationStructures;

    std::vector<MeshInstance> mStaticMeshInstances;
    std::vector<uint32_t>     mFreeStaticMeshIndicies;
    std::vector<MeshInstance> mDynamicMeshInstances;
    std::vector<uint32_t>     mFreeDynamicMeshIndicies;

    OctTree<MeshInstance*> mStaticMeshBoundingVolume;
    OctTree<MeshInstance*> mDynamicMeshBoundingVolume;
    OctTree<Light*>        mLightsBoundingVolume;

    float4x4 mRootTransform;
    AABB mSceneAABB;


	Camera* mSceneCamera;

	std::vector<Material> mMaterials;
	std::vector<ImageView> mMaterialImageViews;

    std::vector<CPUImage> mCPUMaterials;

    std::vector<Light>        mLights;
    std::vector<uint32_t>     mFreeLightIndicies;

    Camera* mShadowLightCamera;
    ShadowingLight mShadowingLight;
    ShadowCascades mCascadesInfo;

    uint64_t mNextInstanceID;
    std::unordered_map<InstanceID, InstanceInfo> mInstanceMap;

	std::unique_ptr<Image> mSkybox;
	std::unique_ptr<ImageView> mSkyboxView;
    std::unique_ptr<CPUImage> mCPUSkybox;

    std::unique_ptr<VoxelTerrain> mTerrain;
};

#endif
