#include "Engine/OverlayTechnique.hpp"
#include "Engine/Engine.hpp"

#include "imgui.h"


OverlayTechnique::OverlayTechnique(Engine* eng) :
	Technique{"Overlay", eng->getDevice()},
	mFontTexture(getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 512, 64, 1, 1, 1, 1, "Font Texture"),
	mFontImageView(mFontTexture, ImageViewType::Colour),
	mOverlayUniformBuffer(getDevice(), vk::BufferUsageFlagBits::eUniformBuffer, 16, 16, "Transformations"),
	mOverlayerBufferView(mOverlayUniformBuffer),
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

	mFontTexture.setContents(pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);
}


void OverlayTechnique::render(RenderGraph& graph, Engine* engine, const std::vector<const Scene::MeshInstance *>&)
{
	ImDrawData* drawData = ImGui::GetDrawData();

	if (!drawData)
		return;

	float transformations[4];
	transformations[0] = 2.0f / drawData->DisplaySize.x;
	transformations[1] = 2.0f / drawData->DisplaySize.y;
	transformations[2] = -1.0f - drawData->DisplayPos.x * transformations[0];
	transformations[3] = -1.0f - drawData->DisplayPos.y * transformations[1];

	MapInfo mapInfo{};
	mapInfo.mOffset = 0;
	mapInfo.mSize = mOverlayUniformBuffer.getSize();
	void* uboPtr = mOverlayUniformBuffer.map(mapInfo);

	memcpy(uboPtr, &transformations[0], 16);

	mOverlayUniformBuffer.unmap();

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

	const auto overlayVertexByteOffset = engine->addVertexData(vertexData.data(), vertexData.size() * sizeof(ImDrawVert));
	const auto indexBufferOffset = engine->addIndexData(indexData);

	GraphicsTask task("ImGuiOverlay", mPipelineDescription);
	task.setVertexAttributes(VertexAttributes::Position2 | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
	task.addInput("OverlayUBO", AttachmentType::UniformBuffer);
	task.addInput(kDefaultFontTexture, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, engine->getSwapChainImage().getFormat(), SizeClass::Custom, LoadOp::Preserve);
	task.setVertexBufferOffset(static_cast<uint32_t>(overlayVertexByteOffset));

	// Render command lists
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = drawData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			task.addIndexedDrawCall(vertexOffset, (indexBufferOffset / sizeof(uint32_t)) + indexOffset, pcmd->ElemCount);

			indexOffset += pcmd->ElemCount;
		}
		vertexOffset += cmd_list->VtxBuffer.Size;
	}

	graph.addTask(task);
}
