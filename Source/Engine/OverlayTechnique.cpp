#include "Engine/OverlayTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

#include "imgui.h"

constexpr const char kOverlayOBU[] = "OverlayUBO";
constexpr const char kOverlayVertex[] = "OverlayVertex";
constexpr const char kOverlayIndex[] = "OverlayIndex";

OverlayTechnique::OverlayTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique{"Overlay", eng->getDevice()},
	mFontTexture(getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 512, 64, 1, 1, 1, 1, "Font Texture"),
	mFontImageView(mFontTexture, ImageViewType::Colour),
	mOverlayUniformBuffer(getDevice(), BufferUsage::Uniform, 16, 16, "Transformations"),
	mOverlayerBufferView(mOverlayUniformBuffer),
    mOverlayVertexBuffer(getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest, 1024 * 1024, 1024 * 1024, "Overlay Vertex Buffer"),
    mOverlayerVertexBufferView((mOverlayVertexBuffer)),
    mOverlayIndexBuffer(getDevice(), BufferUsage::Index | BufferUsage::TransferDest, 1024 * 1024, 1024 * 1024, "Overlay Index Buffer"),
    mOverlayerIndexBufferView((mOverlayIndexBuffer)),
    mPipelineDescription(Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::None, // no backface culling
						 BlendMode::None,
						 BlendMode::None,
						 false, // no depth writes
						 DepthTest::None,
                         FillMode::Fill,
                         Primitive::TriangleList),
    mOverlayVertexShader(eng->getShader("./Shaders/Overlay.vert")),
    mOverlayFragmentShader(eng->getShader("./Shaders/Overlay.frag"))
{
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	mFontTexture->setContents(pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

	GraphicsTask task("ImGuiOverlay", mPipelineDescription);
	task.setVertexAttributes(VertexAttributes::Position2 | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
	task.addInput(kOverlayOBU, AttachmentType::UniformBuffer);
	task.addInput(kDefaultFontTexture, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput(kOverlayVertex, AttachmentType::VertexBuffer);
	task.addInput(kOverlayIndex, AttachmentType::IndexBuffer);
	task.addManagedOutput(kOverlay, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Swapchain, LoadOp::Clear_Black);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
        {
            ImDrawData* drawData = ImGui::GetDrawData();
            if (drawData)
            {
                exec->bindIndexBuffer(*this->mOverlayerIndexBufferView, 0);
                exec->bindVertexBuffer(*this->mOverlayerVertexBufferView, 0);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mOverlayVertexShader, nullptr, nullptr, nullptr, mOverlayFragmentShader);

                // Render command lists
                uint32_t vertexOffset = 0;
                uint32_t indexOffset = 0;
                for (int n = 0; n < drawData->CmdListsCount; n++)
                {
                    const ImDrawList* cmd_list = drawData->CmdLists[n];
                    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                    {
                        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

                        exec->indexedDraw(vertexOffset, indexOffset, pcmd->ElemCount);

                        indexOffset += pcmd->ElemCount;
                    }
                    vertexOffset += cmd_list->VtxBuffer.Size;
                }
            }
        }
    );

	mTaskID = graph.addTask(task);
}


void OverlayTechnique::render(RenderGraph&, RenderEngine*)
{
	(*mOverlayUniformBuffer)->updateLastAccessed();
	mFontTexture->updateLastAccessed();
	mFontImageView->updateLastAccessed();

	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData)
	{
		float transformations[4];
		transformations[0] = 2.0f / drawData->DisplaySize.x;
		transformations[1] = 2.0f / drawData->DisplaySize.y;
		transformations[2] = -1.0f - drawData->DisplayPos.x * transformations[0];
		transformations[3] = -1.0f - drawData->DisplayPos.y * transformations[1];

		MapInfo mapInfo{};
		mapInfo.mOffset = 0;
		mapInfo.mSize = mOverlayUniformBuffer.get()->getSize();
		void* uboPtr = mOverlayUniformBuffer.get()->map(mapInfo);

		memcpy(uboPtr, &transformations[0], 16);

		mOverlayUniformBuffer.get()->unmap();

		const size_t vertexSize = static_cast<size_t>(drawData->TotalVtxCount);

		// ImGui uses uin16_t sized indexs (by default) but we always use 32 bit (this wastes some memory here)
		// so override this in imconfig.h
		const size_t indexSize = static_cast<size_t>(drawData->TotalIdxCount);

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

		if (!vertexData.empty() && !indexData.empty())
		{
			mOverlayVertexBuffer.get()->setContents(vertexData.data(), vertexData.size() * sizeof(ImDrawVert));
			mOverlayIndexBuffer.get()->setContents(indexData.data(), indexData.size() * sizeof(uint32_t));
		}
	}
}

void OverlayTechnique::bindResources(RenderGraph& graph)
{
	graph.bindImage(kDefaultFontTexture, mFontImageView);
	graph.bindBuffer(kOverlayOBU, *mOverlayerBufferView);
	graph.bindBuffer(kOverlayVertex, mOverlayerVertexBufferView.get());
	graph.bindBuffer(kOverlayIndex, mOverlayerIndexBufferView.get());
}
