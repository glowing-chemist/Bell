#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Core/RenderInstance.hpp"
#include "Core/RenderDevice.hpp"

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

    Camera& getCurrentSceneCamera();

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

    void   setImageInScene(const std::string& name, const ImageView& image)
		{ mCurrentRenderGraph.bindImage(name, image); }

	void   setBufferInScene( const std::string& name , const BufferView& buffer)
		{ mCurrentRenderGraph.bindBuffer(name, buffer); }

	void setSamperInScene(const std::string& name, const Sampler sampler)
		{ mCurrentRenderGraph.bindSampler(name, sampler); }

	void setVertexBufferforScene(const Buffer& vertBuf)
		{ mCurrentRenderGraph.bindVertexBuffer(vertBuf); }

	void setIndexBufferforScene(const Buffer& indexBuf)
		{ mCurrentRenderGraph.bindIndexBuffer(indexBuf); }

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

    GLFWwindow* getWindow()
        { return mWindow; }

	RenderDevice* getDevice()
		{ return &mRenderDevice; }

private:

    RenderInstance mRenderInstance;
    RenderDevice mRenderDevice;

    Scene mCurrentScene;
    Scene mLoadingScene;

    BufferBuilder mVertexBuilder;
    uint64_t mOverlayVertexByteOffset;

    BufferBuilder mIndexBuilder;
    uint64_t mOverlayIndexByteOffset;

    RenderGraph mCurrentRenderGraph;

    Shader mOverlayVertexShader;
    Shader mOverlayFragmentShader;

	std::unordered_map<std::string, Shader> mShaderCache;

	// Global uniform buffers

	// Name: CameraBuffer
	CameraBuffer mCameraBuffer;
	Buffer mDeviceCameraBuffer;

	// Name: SSAOBuffer
	SSAOBuffer mSSAOBUffer;
	Buffer mDeviceSSAOBuffer;
	bool mGeneratedSSAOBuffer;

	void updateGlobalUniformBuffers();

	std::unordered_map<PassType, Technique<RenderTask>*> mTechniques;


    std::map<std::string, std::variant<int64_t, double>> mRenderVariables;

    GLFWwindow* mWindow;
};

#endif
