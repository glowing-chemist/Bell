#include "Engine/OverlayTechnique.hpp"
#include "Engine/Engine.hpp"

#include "imgui.h"


OverlayTechnique::OverlayTechnique(Engine* eng, RenderGraph& graph) :
	Technique{"Overlay", eng->getDevice()},
	mFontTexture(getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 512, 64, 1, 1, 1, 1, "Font Texture"),
	mFontImageView(mFontTexture, ImageViewType::Colour),
	mOverlayUniformBuffer(getDevice(), BufferUsage::Uniform, 16, 16, "Transformations"),
	mOverlayerBufferView(mOverlayUniformBuffer),
    mOverlayVertexBuffer(getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest, 1024 * 1024, 1024 * 1024, "Overlay Vertex Buffer"),
    mOverlayerVertexBufferView((mOverlayVertexBuffer)),
    mOverlayIndexBuffer(getDevice(), BufferUsage::Index | BufferUsage::TransferDest, 1024 * 1024, 1024 * 1024, "Overlay Index Buffer"),
    mOverlayerIndexBufferView((mOverlayIndexBuffer)),
	mPipelineDescription(eng->getShader("./Shaders/Overlay.vert"),
						 eng->getShader("./Shaders/Overlay.frag"),
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 false, // no backface culling
						 BlendMode::None,
						 BlendMode::None,
						 false, // no depth writes
						 DepthTest::None,
						 Primitive::TriangleList)
{
	ImGuiIO& io = ImGui::GetIO();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	mFontTexture->setContents(pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

	GraphicsTask task("ImGuiOverlay", mPipelineDescription);
	task.setVertexAttributes(VertexAttributes::Position2 | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
	task.addInput("OverlayUBO", AttachmentType::UniformBuffer);
	task.addInput(kDefaultFontTexture, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput("OverlayVertex", AttachmentType::VertexBuffer);
	task.addInput("OverlayIndex", AttachmentType::IndexBuffer);
	task.addOutput(kOverlay, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Swapchain, LoadOp::Clear_Black);

	mTaskID = graph.addTask(task);
}


void OverlayTechnique::render(RenderGraph& graph, Engine* engine, const std::vector<const Scene::MeshInstance *>&)
{
	(*mOverlayUniformBuffer)->updateLastAccessed();
	mFontTexture->updateLastAccessed();
	mFontImageView->updateLastAccessed();

	ImDrawData* drawData = ImGui::GetDrawData();
	GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));
	task.clearCalls();

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
	}
}
