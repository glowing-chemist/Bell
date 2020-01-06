#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Core/RenderInstance.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/PerFrameResource.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include "Engine/Scene.h"
#include "Engine/Camera.hpp"
#include "Engine/CommandContext.hpp"
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

    // async load a scene in to mLoadingScene
    void loadScene(const std::string& path);

    // make mCurrentScene, mLoadingScene (will wait for the scene to finish loading if not already finished).
    void transitionScene();

    // Will immediatly load a scene in to mCurrentScene waiting for it to finish before returning.
    void setScene(const std::string& path);

    // Set the current Scene, mostly for use by the editor.
    void setScene(Scene&);

    Scene& getScene()
    { return mCurrentScene; }

    const Scene& getScene() const
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

    bool isGraphicsTask(const PassType) const;
    bool isComputeTask(const PassType) const;

	void registerPass(const PassType);
	bool isPassRegistered(const PassType) const;
	void clearRegisteredPasses()
	{
		mTechniques.clear();
		mCurrentPasstypes = 0;
	}

    // returns an vertex and index buffer offset.
    std::pair<uint64_t, uint64_t> addMeshToBuffer(const StaticMesh*);

	uint64_t					  addVertexData(const void* ptr, const size_t size)
		{ return mVertexBuilder.addData(ptr, size); }

	uint64_t					  addIndexData(const std::vector<uint32_t>& idx)
		{ return mIndexBuilder.addData(idx); }

    void   setImageInScene(const std::string& name, const ImageView& image)
		{ mCurrentRenderGraph.bindImage(name, image); }

	void   setBufferInScene( const std::string& name , const BufferView& buffer)
		{ mCurrentRenderGraph.bindBuffer(name, buffer); }

	void setSamperInScene(const std::string& name, const Sampler sampler)
		{ mCurrentRenderGraph.bindSampler(name, sampler); }

	void setVertexBufferforScene(const Buffer& vertBuf)
		{ *mVertexBuffer = vertBuf; }

	void setIndexBufferforScene(const Buffer& indexBuf)
		{ *mIndexBuffer = indexBuf; }

    void recordScene();

	void execute(RenderGraph&);

	void startFrame()
	{
		mRenderDevice->startFrame();
		mCurrentRenderGraph.reset();
	}

	void endFrame()
	{
		mVertexCache.clear();
		mRenderDevice->endFrame(); 
	}

    void render();
	void swap()
		{ mRenderDevice->swap(); }

    void submitCommandRecorder(CommandContext& ccx);

	void flushWait()
	{ mRenderDevice->flushWait(); }

    GLFWwindow* getWindow()
        { return mWindow; }

	RenderDevice* getDevice()
		{ return mRenderDevice; }

private:

	std::unique_ptr<Technique>                   getSingleTechnique(const PassType);

    RenderInstance* mRenderInstance;
    RenderDevice* mRenderDevice;

    Scene mCurrentScene;
    Scene mLoadingScene;

    BufferBuilder mVertexBuilder;
    BufferBuilder mIndexBuilder;

	ShaderResourceSet mMaterials;

	std::unordered_map < const StaticMesh*, std::pair<uint64_t, uint64_t>> mVertexCache;

    RenderGraph mCurrentRenderGraph;
	std::vector<std::unique_ptr<Technique>> mTechniques;
	uint64_t mCurrentPasstypes;

	CommandContext mCommandContext;

	std::unordered_map<std::string, Shader> mShaderCache;

    PerFrameResource<Buffer> mVertexBuffer;
    PerFrameResource<Buffer> mIndexBuffer;

    Sampler mDefaultSampler;

	// Global uniform buffers

	CameraBuffer mCameraBuffer;
    PerFrameResource<Buffer> mDeviceCameraBuffer;

    PerFrameResource<Buffer> mShadowCastingLight;

	SSAOBuffer mSSAOBUffer;
	Buffer mDeviceSSAOBuffer;
	bool mGeneratedSSAOBuffer;

    PerFrameResource<Buffer> mLightBuffer;
    PerFrameResource<BufferView> mLightBufferView;
    PerFrameResource<ShaderResourceSet> mLightsSRS;

    void updateGlobalBuffers();

    std::mutex mSubmissionLock;

    GLFWwindow* mWindow;
};

#endif
