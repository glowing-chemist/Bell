#include "Engine/Engine.hpp"
#include "Core/ConversionUtils.hpp"
#include "Core/BellLogging.hpp"

Engine::Engine(GLFWwindow* windowPtr) :
    mRenderInstance(windowPtr),
    mRenderDevice(mRenderInstance.createRenderDevice(DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Discrete)),
    mCurrentScene("Initial current scene"),
    mLoadingScene("Initial loading scene"),
    mVertexBuilder(),
    mOverlayVertexByteOffset(0),
    mIndexBuilder(),
    mOverlayIndexByteOffset(0),
    mCurrentRenderGraph(),
    mOverlayVertexShader(&mRenderDevice, "./Shaders/Overlay.vert"),
    mOverlayFragmentShader(&mRenderDevice, "./Shaders/Overlay.frag"),
    mVertexBuffer{getDevice(), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 1000, 1000, "Vertex Buffer"},
    mIndexBuffer{getDevice(), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, 1000, 1000, "Index Buffer"},
	mCameraBuffer{},
	mDeviceCameraBuffer{getDevice(), vk::BufferUsageFlagBits::eUniformBuffer, sizeof(CameraBuffer), sizeof(CameraBuffer), "Camera Buffer"},
	mSSAOBUffer{},
	mDeviceSSAOBuffer{getDevice(), vk::BufferUsageFlagBits::eUniformBuffer, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Buffer"},
	mGeneratedSSAOBuffer{false},
    mRenderVariables(),
    mWindow(windowPtr)
{
    mOverlayVertexShader.compile();
    mOverlayFragmentShader.compile();
}


void Engine::loadScene(const std::string& path)
{
    mLoadingScene = Scene(path);
    mLoadingScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals);
}

void Engine::transitionScene()
{
    mCurrentScene = std::move(mLoadingScene);
}


void Engine::setScene(const std::string& path)
{
    mCurrentScene = Scene(path);
    // For now don't include material ID, some more work will be needed to expose that correctly.
    mCurrentScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals);
}


void Engine::setScene(Scene& scene)
{
    mCurrentScene = std::move(scene);
}


Camera& Engine::getCurrentSceneCamera()
{
    return mCurrentScene.getCamera();
}


Image Engine::createImage(const uint32_t x,
				  const uint32_t y,
				  const uint32_t z,
                  const uint32_t mips,
                  const uint32_t levels,
                  const uint32_t samples,
				  const Format format,
				  ImageUsage usage,
				  const std::string& name)
{
    return Image{&mRenderDevice, format, usage, x, y, z, mips, levels, samples, name};
}

Buffer Engine::createBuffer(const uint32_t size,
					const uint32_t stride,
					vk::BufferUsageFlags usage,
					const std::string& name)
{
	return Buffer{&mRenderDevice, usage, size, stride, name};
}


Shader Engine::getShader(const std::string& path)
{
	if(mShaderCache.find(path) != mShaderCache.end())
		return (*mShaderCache.find(path)).second;

	Shader newShader{&mRenderDevice, path};

	const bool compiled = newShader.compile();

	BELL_ASSERT(compiled, "Shader failed to compile")

	mShaderCache.insert({path, newShader});

	return newShader;
}


void Engine::recordOverlay(const ImDrawData* drawData)
{
	const size_t vertexSize = drawData->TotalVtxCount;

    // ImGui uses uin16_t sized indexs (by default) but we always use 32 bit (this wastes some memory here)
    // so override this in imconfig.h
	const size_t indexSize = drawData->TotalIdxCount;

	std::vector<ImDrawVert> vertexData(vertexSize);
	std::vector<uint32_t> indexData(indexSize);

	ImDrawVert* vertexPtr = vertexData.data();
	ImDrawIdx* indexPtr = indexData.data();

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        memcpy(vertexPtr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(indexPtr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertexPtr += cmd_list->VtxBuffer.Size;
		indexPtr += cmd_list->IdxBuffer.Size;
    }

    mOverlayVertexByteOffset = mVertexBuilder.addData(vertexData);
    mOverlayIndexByteOffset = mIndexBuilder.addData(indexData);

    Rect scissorRect{mRenderDevice.getSwapChain()->getSwapChainImageWidth(),
                     mRenderDevice.getSwapChain()->getSwapChainImageHeight()};

    Rect viewport{mRenderDevice.getSwapChain()->getSwapChainImageWidth(),
                  mRenderDevice.getSwapChain()->getSwapChainImageHeight()};

	GraphicsPipelineDescription desc(mOverlayVertexShader,
                                     mOverlayFragmentShader,
                                     scissorRect,
                                     viewport,
                                     false, // no backface culling
									 BlendMode::None,
									 BlendMode::None,
                                     false, // no depth writes
									 DepthTest::None);

    GraphicsTask task("ImGuiOverlay", desc);
    task.setVertexAttributes(VertexAttributes::Position2 | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
	task.addInput("OverlayUBO", AttachmentType::UniformBuffer);
    task.addInput("OverlayTexture", AttachmentType::Texture2D);
    task.addInput("FontSampler", AttachmentType::Sampler);
	task.addOutput("FrameBuffer", AttachmentType::SwapChain, getSwapChainImage().getFormat(), LoadOp::Clear_Black);

    // Render command lists
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            task.addIndexedDrawCall(vertexOffset, indexOffset, pcmd->ElemCount);

            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }

    mCurrentRenderGraph.addTask(task);
}


void Engine::recordScene()
{
}


void Engine::render()
{
	auto& vertexData = mVertexBuilder.finishRecording();

	auto& indexData = mIndexBuilder.finishRecording();

    mVertexBuffer->resize(static_cast<uint32_t>(vertexData.size()), false);
    mIndexBuffer->resize(static_cast<uint32_t>(indexData.size()), false);

    mVertexBuffer->setContents(vertexData.data(), static_cast<uint32_t>(vertexData.size()));
    mIndexBuffer->setContents(indexData.data(), static_cast<uint32_t>(indexData.size()));

    mVertexBuilder.reset();
    mIndexBuilder.reset();

    mCurrentRenderGraph.bindVertexBuffer(*mVertexBuffer);
    mCurrentRenderGraph.bindIndexBuffer(*mIndexBuffer);

    mRenderDevice.execute(mCurrentRenderGraph);
}


void Engine::updateGlobalUniformBuffers()
{
	auto& currentCamera = getCurrentSceneCamera();

	mCameraBuffer.mViewMatrix = currentCamera.getViewMatrix();
	mCameraBuffer.mPerspectiveMatrix = currentCamera.getPerspectiveMatrix();
	mCameraBuffer.mInvertedCameraMatrix = glm::inverse(mCameraBuffer.mPerspectiveMatrix * mCameraBuffer.mViewMatrix);
	mCameraBuffer.mFrameBufferSize = glm::vec2{getSwapChainImage().getExtent(0, 0).width, getSwapChainImage().getExtent(0, 0).height};
	mCameraBuffer.mPosition = currentCamera.getPosition();

	MapInfo mapInfo{};
	mapInfo.mSize = sizeof(CameraBuffer);
	mapInfo.mOffset = 0;

    void* cameraBufferPtr = mDeviceCameraBuffer->map(mapInfo);

		std::memcpy(cameraBufferPtr, &mCameraBuffer, sizeof(CameraBuffer));

    mDeviceCameraBuffer->unmap();

	// The SSAO buffer only needs tp be updated once.
	// Or if the number of samples needs to be changed.
	if(!mGeneratedSSAOBuffer)
	{
		std::array<float3, 16> offsets;

		for(uint32_t i = 0; i < 16; ++i)
		{
			const float r1 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;
			const float r2 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)); // generate vectors on a hemisphere.
			const float r3 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;

			const float3 vec = glm::normalize(float3{r1, r2, r3});

			offsets[i] = float4{vec, 0.0f};
		}

		mGeneratedSSAOBuffer = true;

        std::memcpy(&mSSAOBUffer.mOffsets[0], offsets.data(), offsets.size() * sizeof(float3));
		mSSAOBUffer.mScale = 0.001f;
		mSSAOBUffer.mOffsetsCount = offsets.size();

		mapInfo.mSize = sizeof(SSAOBuffer);

		void* SSAOBufferPtr = mDeviceSSAOBuffer.map(mapInfo);

			std::memcpy(SSAOBufferPtr, &mSSAOBUffer, sizeof(SSAOBuffer));

		mDeviceSSAOBuffer.unmap();
	}
}
