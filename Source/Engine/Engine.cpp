#include "Engine/Engine.hpp"


Engine::Engine(GLFWwindow* windowPtr) :
    mRenderInstance(windowPtr),
    mRenderDevice(mRenderInstance.createRenderDevice(DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Discrete)),
    mCurrentScene("Initial current scene"),
    mLoadingScene("Initial loading scene"),
    mCurrentRenderGraph(),
    mOverlayVertexShader(&mRenderDevice, "Shaders/Overlay.Vert"),
    mOverlayFragmentShader(&mRenderDevice, "Shaders/Overlay.frag"),
    mRenderVariables(),
    mWindow(windowPtr)
{
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


void Engine::recordOverlay(const ImDrawData* drawData)
{
    const size_t vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);

    // ImGui uses uin16_t sized indexs (by default) but we always use 32 bit (this wastes some memory here)
    // so override this in imconfig.h
    const size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    std::vector<unsigned char> vertexData(vertexSize);
    std::vector<uint32_t> indexData(indexSize);

    unsigned char* vertexPtr = vertexData.data();
    unsigned char* indexPtr = reinterpret_cast<unsigned char*>(indexData.data());

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
                                     DepthTest::None};

    GraphicsTask task("ImGuiOverlay", desc);
    task.setVertexAttributes(VertexAttributes::Position | VertexAttributes::TextureCoordinates | VertexAttributes::Aledo);
    task.addInput("OverlayTexture", AttachmentType::Texture2D);
    task.addOutput("FrameBuffer", AttachmentType::SwapChain);

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
    mRenderDevice.execute(mCurrentRenderGraph);
}
