#include "Engine/Engine.hpp"
#include "Core/ConversionUtils.hpp"


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
    mRenderVariables(),
    mWindow(windowPtr)
{
    mOverlayVertexShader.compile();
    mOverlayFragmentShader.compile();
}


void Engine::loadScene(const std::string& path)
{

}

void Engine::transitionScene()
{

}


void Engine::setScene(const std::string& path)
{

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
				  const vk::Format format,
				  vk::ImageUsageFlags usage,
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

    GraphicsPipelineDescription desc{mOverlayVertexShader,
                                     {}, // We're not using a geom/tess shaders for the overlay, duh.
                                     {},
                                     {},
                                     mOverlayFragmentShader,
                                     scissorRect,
                                     viewport,
                                     false, // no backface culling
									 BlendMode::None,
									 BlendMode::None,
                                     DepthTest::None};

    GraphicsTask task("ImGuiOverlay", desc);
	task.setVertexAttributes(VertexAttributes::Position2 | VertexAttributes::TextureCoordinates | VertexAttributes::Aledo);
	task.addInput("OverlayUBO", AttachmentType::UniformBuffer);
    task.addInput("OverlayTexture", AttachmentType::Texture2D);
    task.addInput("FontSampler", AttachmentType::Sampler);
	task.addOutput("FrameBuffer", AttachmentType::SwapChain, getBellImageFormat(getSwapChainImage().getFormat()), LoadOp::Clear_Black);

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

	Buffer vertexBuffer = createBuffer(vertexData.size(), vertexData.size(), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, "Vertex Buffer");
	vertexBuffer.setContents(vertexData.data(), vertexData.size());

	auto& indexData = mIndexBuilder.finishRecording();

	Buffer indexBuffer = createBuffer(indexData.size(), indexData.size(), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, "Index Buffer");
	indexBuffer.setContents(indexData.data(), indexData.size());

	mVertexBuilder = BufferBuilder();
	mIndexBuilder = BufferBuilder();

	mCurrentRenderGraph.bindVertexBuffer(vertexBuffer);
	mCurrentRenderGraph.bindIndexBuffer(indexBuffer);

    mRenderDevice.execute(mCurrentRenderGraph);
}
