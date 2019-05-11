#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Core/RenderInstance.hpp"
#include "Core/RenderDevice.hpp"

#include "RenderGraph/RenderGraph.hpp"

#include "Engine/Scene.h"
#include "Engine/Camera.hpp"
#include "Engine/Bufferbuilder.h"

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

	Image createImage(const uint32_t x,
					  const uint32_t y,
					  const uint32_t z,
					  const vk::Format,
					  vk::ImageUsageFlags,
					  const std::string& = "");

	Buffer createBuffer(const uint32_t size,
						const uint32_t stride,
						vk::BufferUsageFlags,
						const std::string& = "");

	void   setImageInScene(const std::string& name, const Image& image)
		{ mCurrentRenderGraph.bindImage(name, image); }

	void   setBufferInScene( const std::string& name , const Buffer& buffer)
		{ mCurrentRenderGraph.bindBuffer(name, buffer); }

	void setVertexBufferforScene(const Buffer& vertBuf)
		{ mCurrentRenderGraph.bindVertexBuffer(vertBuf); }

	void setIndexBufferforScene(const Buffer& indexBuf)
		{ mCurrentRenderGraph.bindIndexBuffer(indexBuf); }

    void recordOverlay(const ImDrawData*);

    void recordScene();

	void startFrame()
		{ mRenderDevice.startFrame(); }

	void endFrame()
		{ mRenderDevice.endFrame(); }

    void render();
	void swap()
		{ mRenderDevice.swap(); }

    GLFWwindow* getWindow()
        { return mWindow; }

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

    std::map<std::string, std::variant<int64_t, double>> mRenderVariables;

    GLFWwindow* mWindow;
};

#endif
