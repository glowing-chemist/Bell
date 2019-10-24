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

// Render paths supported by the engine.
enum class RenderType
{
	BindlessForwardPlus,
	BindlessDeferred
};

enum class LightingType
{
	BlinnPhong,
	PBR
};


struct RenderOptions
{
	RenderOptions() :
		mRenderType{RenderType::BindlessDeferred},
		mLightingType{LightingType::PBR},
		mUseSSAO{false},
		mRenderOverlay{false} {}

	RenderType mRenderType;
	LightingType mLightingType;

	bool mUseSSAO;
	bool mRenderOverlay;
};


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

    Camera& getCurrentSceneCamera();

    BufferBuilder& getVertexBufferBuilder()
    { return mVertexBuilder; }

    BufferBuilder& getIndexBufferBuilder()
    { return mIndexBuilder; }

	Image&  getSwapChainImage()
		{ return mRenderDevice.getSwapChainImage(); }

    ImageView& getSwaChainImageView()
        { return mRenderDevice.getSwapChainImageView(); }

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
						vk::BufferUsageFlags,
						const std::string& = "");

	Shader getShader(const std::string& path);

    bool isGraphicsTask(const PassType) const;
    bool isComputeTask(const PassType) const;

	void registerPass(const PassType);

    // returns an vertex and index buffer offset.
    std::pair<uint64_t, uint64_t> addMeshToBuffer(const StaticMesh*);

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

    void recordOverlay(const ImDrawData*);

    void recordScene();

	void startFrame()
	{
		mRenderDevice.startFrame();
		mCurrentRenderGraph.reset();
	}

	void endFrame()
		{ mRenderDevice.endFrame(); }

    void render();
	void swap()
		{ mRenderDevice.swap(); }

    void submitCommandRecorder(CommandContext& ccx);

	void flushWait()
	{ mRenderDevice.flushWait(); }

    GLFWwindow* getWindow()
        { return mWindow; }

	RenderDevice* getDevice()
		{ return &mRenderDevice; }

private:

	void renderSceneBindlessDefferred(const LightingType);
	void renderSceneBindlessForwardPlus(const LightingType);

	std::unique_ptr<Technique>                   getSingleTechnique(const PassType);

    RenderInstance mRenderInstance;
    RenderDevice mRenderDevice;

    Scene mCurrentScene;
    Scene mLoadingScene;

    BufferBuilder mVertexBuilder;
    uint64_t mOverlayVertexByteOffset;

    BufferBuilder mIndexBuilder;
    uint64_t mOverlayIndexByteOffset;

    RenderGraph mCurrentRenderGraph;
	std::vector<std::unique_ptr<Technique>> mTechniques;

	CommandContext mCommandContext;

    Shader mOverlayVertexShader;
    Shader mOverlayFragmentShader;

	std::unordered_map<std::string, Shader> mShaderCache;

    PerFrameResource<Buffer> mVertexBuffer;
    PerFrameResource<Buffer> mIndexBuffer;

	// Global uniform buffers

	// Name: CameraBuffer
	CameraBuffer mCameraBuffer;
    PerFrameResource<Buffer> mDeviceCameraBuffer;

	// Name: SSAOBuffer
	SSAOBuffer mSSAOBUffer;
	Buffer mDeviceSSAOBuffer;
	bool mGeneratedSSAOBuffer;

	void updateGlobalUniformBuffers();

	RenderOptions mRenderOptions;

    std::mutex mSubmissionLock;

    GLFWwindow* mWindow;
};

#endif
