#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Core/RenderInstance.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/PerFrameResource.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include "Engine/Scene.h"
#include "Engine/Camera.hpp"
#include "Engine/Bufferbuilder.h"
#include "Engine/UniformBuffers.h"
#include "Engine/Technique.hpp"

#include "imgui.h"

#include "GLFW/glfw3.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <variant>


class Engine
{
public:

    Engine(GLFWwindow*);

    void setScene(const std::string& path);

    // Set the current Scene, mostly for use by the editor.
    void setScene(Scene*);

    Scene* getScene()
    { return mCurrentScene; }

    const Scene* getScene() const
    { return mCurrentScene; }

    Camera& getCurrentSceneCamera();

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

    bool isGraphicsTask(const PassType) const;
    bool isComputeTask(const PassType) const;

	void registerPass(const PassType);
	bool isPassRegistered(const PassType) const;
	void clearRegisteredPasses()
	{
		mTechniques.clear();
        mShaderPrefix.clear();
		mCurrentRegistredPasses = 0;
        mCurrentRenderGraph.reset();
        mCompileGraph = true;
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

    void startFrame()
    {
    mRenderDevice->startFrame();
    }

    void endFrame()
    {
	mRenderDevice->endFrame();
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

    struct AnimationEntry
    {
	std::string mName;
	InstanceID mMesh;
	float mSpeedModifier;
	uint64_t mBoneOffset;
	double mTick;
	bool mLoop;
    };

    const std::vector<AnimationEntry>& getActiveAnimations() const
    {
        return mActiveAnimations;
    }

private:

    std::unique_ptr<Technique>                   getSingleTechnique(const PassType);

    RenderInstance* mRenderInstance;
    RenderDevice* mRenderDevice;

    Scene* mCurrentScene;

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

    std::unordered_map < const StaticMesh*, std::pair<uint64_t, uint64_t>> mVertexCache;
    std::unordered_map < const StaticMesh*, std::pair<uint64_t, uint64_t>> mTposeVertexCache;

    RenderGraph mCurrentRenderGraph;
    bool mCompileGraph;
    std::vector<std::unique_ptr<Technique>> mTechniques;
    uint64_t mPassesRegisteredThisFrame;
    uint64_t mCurrentRegistredPasses;
    std::vector<std::string> mShaderPrefix; // Containes defines for currently registered passes.


    std::unordered_map<uint64_t, Shader> mShaderCache;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;
    // Animation data.
    Buffer mTposeVertexBuffer; // a "clean" copy of animated meshed verticies.
    Buffer mBonesWeightsBuffer;
    Buffer mBoneWeightsIndexBuffer;
    PerFrameResource<Buffer> mBoneBuffer;

    Sampler mDefaultSampler;

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

    std::vector<AnimationEntry> mActiveAnimations;
    void tickAnimations();
    std::chrono::system_clock::time_point mAnimationLastTicked;

    void updateGlobalBuffers();

    uint32_t mMaxCommandThreads;
    std::mutex mSubmissionLock;

    GLFWwindow* mWindow;
};

#endif
