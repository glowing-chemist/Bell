#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Core/RenderInstance.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/PerFrameResource.hpp"
#include "Core/ContainerUtils.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include "Engine/Scene.h"
#include "RayTracedScene.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Bufferbuilder.h"
#include "Engine/UniformBuffers.h"
#include "Engine/Technique.hpp"
#include "Engine/ThreadPool.hpp"

#include "imgui.h"

#include "GLFW/glfw3.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <variant>


class Engine
{
public:

    Engine(GLFWwindow*);

    void setScene(const std::string& path);

    // Set the current Scene, mostly for use by the editor.
    void setScene(Scene*);
    void setRayTracingScene(RayTracingScene* scene);

    Scene* getScene()
    { return mCurrentScene; }

    const Scene* getScene() const
    { return mCurrentScene; }

    Camera& getCurrentSceneCamera();

    void setDebugCameraActive(const bool debugCam)
    {
        mDebugCameraActive = debugCam;
    }

    bool getDebugCameraActive() const
    {
        return mDebugCameraActive;
    }

    BufferBuilder& getVertexBufferBuilder()
    { return mVertexBuilder; }

    BufferBuilder& getIndexBufferBuilder()
    { return mIndexBuilder; }

	Image&  getSwapChainImage()
		{ return mRenderDevice->getSwapChainImage(); }

	ImageView& getSwapChainImageView()
        { return mRenderDevice->getSwapChainImageView(); }

	Image createImage(const uint32_t x,
					  const uint32_t y,
					  const uint32_t z,
					  const uint32_t mips,
					  const uint32_t levels,
					  const uint32_t samples,
					  const Format,
					  ImageUsage,
					  const std::string& = "");

	Buffer createBuffer(const uint32_t size,
						const uint32_t stride,
						BufferUsage,
						const std::string& = "");

	Shader getShader(const std::string& path);
    Shader getShader(const std::string& path, const ShaderDefine& define);

    const Buffer& getShadowBuffer() const
    {
        return mShadowCastingLight.get();
    }

    const RenderGraph& getRenderGraph() const
    {
        return mCurrentRenderGraph;
    }

    RenderGraph& getRenderGraph()
    {
        return mCurrentRenderGraph;
    }

	void registerPass(const PassType);
	bool isPassRegistered(const PassType) const;
	void clearRegisteredPasses()
	{
		mTechniques.clear();
        mShaderPrefix.clear();
		mCurrentRegistredPasses = 0;
        mCurrentRenderGraph.reset();
        mCompileGraph = true;
        mRecordTasksSync = true; // Need to regenerate async task order info, so record task synchronously once.
	}

    void registerCustomPass(std::unique_ptr<Technique>& technique)
    {
        mTechniques.push_back(std::move(technique));
    }

    void enableDebugTexture(const char* slot)
    {
        mShowDebugTexture = true;
        mDebugTextureName = slot;
    }

    void disableDebugTexture()
    {
        mShowDebugTexture = false;
        mDebugTextureName = "";
    }

    const char* getDebugTextureSlot() const
    {
        return mDebugTextureName;
    }

    bool debugTextureEnabled() const
    {
        return mShowDebugTexture;
    }

    // returns an vertex and index buffer offset.
    std::pair<uint64_t, uint64_t> addMeshToBuffer(const StaticMesh*);
    std::pair<uint64_t, uint64_t> addMeshToAnimationBuffer(const StaticMesh*);
    uint64_t                      getMeshBoundsIndex(const MeshInstance*);

	uint64_t					  addVertexData(const void* ptr, const size_t size)
		{ return mVertexBuilder.addData(ptr, size); }

	uint64_t					  addIndexData(const std::vector<uint32_t>& idx)
		{ return mIndexBuilder.addData(idx); }

    void clearVertexCache() // To be used before uploading new vertex data.
    {
        mVertexCache.clear();
        mTposeVertexCache.clear();
    }

	void setVertexBufferforScene(const Buffer& vertBuf)
        { mVertexBuffer = vertBuf; }

	void setIndexBufferforScene(const Buffer& indexBuf)
        { mIndexBuffer = indexBuf; }

    Buffer& getVertexBuffer()
    {  return mVertexBuffer; }

    const Buffer& getVertexBuffer() const
    {
        return mVertexBuffer;
    }

    Buffer& getIndexBuffer()
    {
        return mIndexBuffer;
    }

    const Buffer& getIndexBuffer() const
    {
        return mIndexBuffer;
    }

    void startAnimation(const InstanceID id, const std::string& name, const bool loop = false, const float speedModifer = 1.0f);
    void terimateAnimation(const InstanceID id, const std::string& name);

    void recordScene();

    void execute(RenderGraph&);

    void startFrame(const std::chrono::microseconds& frameDelta)
    {
    mRenderDevice->startFrame();

    mFrameUpdateDelta = frameDelta;
    // Start timestamp after swapchain wait ie don't count waiting for vsync in frametime.
    mFrameStartTime = std::chrono::system_clock::now();
    }

    void endFrame()
    {
	mRenderDevice->endFrame();

    // Set the frame time.
    mAccumilatedFrameUpdates += mFrameUpdateDelta;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    mLastFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - mFrameStartTime);
    }

    void render();
    void swap()
    { mRenderDevice->swap(); }

    void flushWait()
    { mRenderDevice->flushWait(); }

    GLFWwindow* getWindow()
    { return mWindow; }

    RenderDevice* getDevice()
    { return mRenderDevice; }

    struct SkeletalAnimationEntry
    {
	    std::string mName;
	    InstanceID mMesh;
	    float mSpeedModifier;
	    uint64_t mBoneOffset;
	    double mTick;
	    bool mLoop;
    };

    struct BlendShapeAnimationEntry
    {
        std::string mName;
        InstanceID mMesh;
        float mSpeedModifier;
        double mTick;
        bool mLoop;

    };

    const std::vector<SkeletalAnimationEntry>& getActiveSkeletalAnimations() const
    {
        return mActiveSkeletalAnimations;
    }

    const std::vector<BlendShapeAnimationEntry>& getActiveBlendShapesAnimations() const
    {
        return mActiveBlendShapeAnimations;
    }

    std::chrono::microseconds getLastFrameTime() const
    {
        return mLastFrameTime;
    }

    Technique* getRegisteredTechnique(const PassType);

    void rayTraceScene();

    struct IrradianceProbeVolume
    {
        IrradianceProbeVolume() :
            mProbeDensity(1.0f, 1.0f, 1.0f),
            mBoundingBox({float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f)},
                         float3(0.5f, 0.5f, 0.5f),
                         float3(-0.5f, -0.5f, -0.5f)) {}

        float3 mProbeDensity;
        OBB mBoundingBox;

        std::vector<float3> getProbePositions() const;
    };

    struct RadianceProbe
    {
        RadianceProbe() :
            mPosition{0.0f, 0.0f, 0.0f},
            mRadius(10.0f) {}

        float3 mPosition;
        float mRadius;
    };

    struct SphericalHarmonic
    {
        float4 mPosition;
        float mCoefs[28];
    };

    struct KdNode
    {
        float4 mPivotPoint;
        uint32_t mPivotIndex;
        int32_t mLeftIndex;
        int32_t mRightIndex;
        uint2 m_padding;
    };

    std::vector<SphericalHarmonic> generateIrradianceProbes(const std::vector<IrradianceProbeVolume>& positions);
    std::vector<SphericalHarmonic> generateIrradianceProbes(const std::vector<float3> &positions);
    std::vector<KdNode> generateProbeKdTree(std::vector<SphericalHarmonic> &harmonics);

    const std::vector<SphericalHarmonic>& getIrradianceProbes() const
    {
        return mIrradianceProbesHarmonics;
    }

    void loadIrradianceProbes(const std::string& probesPath, const std::string& lookupPath);

    const std::vector<IrradianceProbeVolume>& getIrradianceVolumes() const
    {
        return mIrradianceVolumes;
    }

    void setIrradianceVolumes(const std::vector<IrradianceProbeVolume>& volumes)
    {
        mIrradianceVolumes = volumes;
    }

    const std::vector<RadianceProbe>& getRadianceProbes() const
    {
        return mRadianceProbes;
    }

    void setRadianceProbes(const std::vector<RadianceProbe>& probes)
    {
        mRadianceProbes = probes;
    }

    // Default available meshes.
    const StaticMesh& getUnitSphereMesh() const
    {
        return *mUnitSphere;
    }

    void setShadowMapResolution(const float2& res)
    {
        mShadowMapResolution = res;
    }

    const float2& getShadowMapResolution() const
    {
        return mShadowMapResolution;
    }

private:

    CPUImage renderDiffuseCubeMap(const RayTracingScene &scene, const float3 &position, const uint32_t x, const uint32_t y);
    SphericalHarmonic generateSphericalHarmonic(const float3 &position, const CPUImage& cubemap);

    std::unique_ptr<Technique>                   getSingleTechnique(const PassType);

    ThreadPool mThreadPool;

    RenderInstance* mRenderInstance;
    RenderDevice* mRenderDevice;

    Scene* mCurrentScene;
    RayTracingScene* mRayTracedScene;

    bool mDebugCameraActive;
    Camera mDebugCamera;

    BufferBuilder mAnimationVertexBuilder;
    BufferBuilder mBoneIndexBuilder;
    BufferBuilder mBoneWeightBuilder;
    BufferBuilder mVertexBuilder;
    BufferBuilder mIndexBuilder;

    ShaderResourceSet mMaterials;

    Image mLTCMat;
    ImageView mLTCMatView;
    Image mLTCAmp;
    ImageView mLTCAmpView;
    bool mInitialisedTLCTextures;
    Image mBlueNoise;
    ImageView mBlueNoiseView;
    Image mDefaultDiffuseTexture;
    ImageView mDefaultDiffuseView;

    std::unordered_map < const StaticMesh*, std::pair<uint64_t, uint64_t>> mVertexCache;
    std::unordered_map < const StaticMesh*, std::pair<uint64_t, uint64_t>> mTposeVertexCache;
    std::unordered_map < const MeshInstance*, uint64_t>                    mMeshBoundsCache;

    RenderGraph mCurrentRenderGraph;
    bool mCompileGraph;
    std::vector<std::unique_ptr<Technique>> mTechniques;
    uint64_t mPassesRegisteredThisFrame;
    uint64_t mCurrentRegistredPasses;
    std::vector<ShaderDefine> mShaderPrefix; // Containes defines for currently registered passes.

    std::shared_mutex mShaderCacheMutex;
    std::unordered_map<uint64_t, Shader> mShaderCache;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;
    // Animation data.
    Buffer mTposeVertexBuffer; // a "clean" copy of animated meshed verticies.
    Buffer mBonesWeightsBuffer;
    Buffer mBoneWeightsIndexBuffer;
    PerFrameResource<Buffer> mBoneBuffer;
    Buffer mMeshBoundsBuffer;

    Sampler mDefaultSampler;
    Sampler mDefaultPointSampler;

    float2 mShadowMapResolution;

    // Debug helpers
    bool mShowDebugTexture;
    const char* mDebugTextureName;

	// Global uniform buffers

    CameraBuffer mCameraBuffer;
    PerFrameResource<Buffer> mDeviceCameraBuffer;

    PerFrameResource<Buffer> mShadowCastingLight;

    PerFrameResource<Buffer> mLightBuffer;
    PerFrameResource<BufferView> mLightBufferView;
    PerFrameResource<BufferView> mLightCountView;
    PerFrameResource<ShaderResourceSet> mLightsSRS;

    float2 mTAAJitter[16];

    std::vector<SkeletalAnimationEntry> mActiveSkeletalAnimations;
    std::vector<BlendShapeAnimationEntry> mActiveBlendShapeAnimations;
    void tickAnimations();

    std::chrono::system_clock::time_point mFrameStartTime;
    std::chrono::microseconds mLastFrameTime;

    std::chrono::microseconds mFrameUpdateDelta;
    std::chrono::microseconds mAccumilatedFrameUpdates;

    void updateGlobalBuffers();

    uint32_t mMaxCommandThreads;
    std::mutex mSubmissionLock;

    std::vector<SphericalHarmonic> mIrradianceProbesHarmonics;
    std::vector<IrradianceProbeVolume> mIrradianceVolumes;
    std::vector<RadianceProbe> mRadianceProbes;
    std::unique_ptr<Buffer> mIrradianceProbeBuffer;
    std::unique_ptr<BufferView> mIrradianceProbeBufferView;
    std::unique_ptr<Buffer> mIrradianceKdTreeBuffer;
    std::unique_ptr<BufferView> mIrradianceKdTreeBufferView;
    ShaderResourceSet mLightProbeResourceSet;

    std::unique_ptr<StaticMesh> mUnitSphere;

    // State for trackin g async task recording.
    bool mRecordTasksSync;
    struct ContextMapping
    {
        StaticGrowableBuffer<uint32_t, 32> mTaskIndicies;
    };
    std::vector<ContextMapping> mAsyncTaskContextMappings;
    std::vector<ContextMapping> mSyncTaskContextMappings;

    GLFWwindow* mWindow;
};

#endif
