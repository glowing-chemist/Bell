
#include "Editor/Editor.h"

#include "imgui.h"

namespace
{

    std::shared_ptr<EditorNode> createPassNode(const uint64_t passType)
    {
        const PassType pType = static_cast<PassType>(passType);

        switch(pType)
        {
            case PassType::DepthPre:
            {
                // Set the ID to 0 as the editor will set the real id.
                std::shared_ptr<EditorNode> newNode = std::make_shared<EditorNode>(0, "Depth Pre", passType);
                newNode->mOutputs.emplace_back(0, newNode, "Depth", PinType::Texture, PinKind::Output);
                return newNode;
            }


            case PassType::GBuffer:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<EditorNode>(0, "GBuffer", passType);
                newNode->mOutputs.push_back(Pin{0, newNode, "Normal", PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, "Albedo", PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, "Specular", PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, "Depth", PinType::Texture, PinKind::Output});
                return newNode;
            }

            case PassType::SSAO:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<EditorNode>(0, "SSAO", passType);
                newNode->mInputs.push_back(Pin{0, newNode, "Depth", PinType::Texture, PinKind::Input});
                newNode->mOutputs.push_back(Pin{0, newNode, "SSAO", PinType::Texture, PinKind::Output});
                return newNode;
            }

        }
    }

}


Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
    mMode{0},
    mShowHelpMenu{false},
    mShowFileBrowser{false},
    mFileBrowser{"/"},
    mShowNodeEditor{false},
    mNodeEditor{"render graph editor", createPassNode},
    mEngine{mWindow},
	mHasUploadedFonts(false),
	mOverlayFontTexture(mEngine.getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 512, 64, 1, 1, 1, 1, "Font Texture"),
	mOverlayTextureView(mOverlayFontTexture, ImageViewType::Colour),
	mUITexture(mEngine.getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::ColourAttachment, 1600, 900, 1, 1, 1, 1, "UI Texture"),
	mUIImageView(mUITexture, ImageViewType::Colour),
	mOverlayTranslationUBO(mEngine.getDevice(), vk::BufferUsageFlagBits::eUniformBuffer, 16, 16, "Transformations"),
	mFontsSampler(SamplerType::Linear),
    mInProgressScene{"In construction"}
{
    ImGui::CreateContext();

	mFontsSampler.setAddressModeU(AddressMode::Repeat);
	mFontsSampler.setAddressModeV(AddressMode::Repeat);
	mFontsSampler.setAddressModeW(AddressMode::Repeat);
}


Editor::~Editor()
{
     ImGui::DestroyContext();
}


void Editor::run()
{
    while(!glfwWindowShouldClose(mWindow))
    {
        pumpInputQueue();

		startFrame();

        // Only draw the scene is we're in scene mode else draw the
        // pass/shader graphs.
        if(mMode == 0)
            renderScene();

		renderOverlay();

		swap();

		endFrame();
	}
}

void Editor::renderScene()
{

}


void Editor::renderOverlay()
{
	ImGuiIO& io = ImGui::GetIO();

	if(!mHasUploadedFonts)
	{
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        mOverlayFontTexture.setContents(pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

		mHasUploadedFonts = true;
	}

    ImGui::NewFrame();

    drawMenuBar();
    drawAssistantWindow();

    // We are on Pass/shader graph mode
    if(mMode == 1)
        mNodeEditor.draw();

    if(mShowFileBrowser)
    {
        ImGui::SetNextWindowPos(ImVec2(800, 450));
        auto optionalPath = mFileBrowser.render();

        if(optionalPath)
            mShowFileBrowser = false;
    }

    // Set up the draw data/ also calls endFrame.
	ImGui::Render();

	// Set up transformation UBO
	ImDrawData* draw_data = ImGui::GetDrawData();

	float transformations[4];
	transformations[0] = 2.0f / draw_data->DisplaySize.x;
	transformations[1] = 2.0f / draw_data->DisplaySize.y;
	transformations[2] = -1.0f - draw_data->DisplayPos.x * transformations[0];
	transformations[3] = -1.0f - draw_data->DisplayPos.y * transformations[1];

	MapInfo mapInfo{};
	mapInfo.mOffset = 0;
	mapInfo.mSize = mOverlayTranslationUBO.getSize();
	void* uboPtr = mOverlayTranslationUBO.map(mapInfo);

	memcpy(uboPtr, &transformations[0], 16);

	mOverlayTranslationUBO.unmap();

	mEngine.recordOverlay(draw_data);

	mEngine.setSamperInScene("FontSampler", mFontsSampler);
    mEngine.setImageInScene("OverlayTexture", mOverlayTextureView);
    mEngine.setImageInScene("FrameBuffer", mEngine.getSwaChainImageView());
	mEngine.setBufferInScene("OverlayUBO", mOverlayTranslationUBO);
}


void Editor::swap()
{
	mEngine.render();
	mEngine.swap();
}


void Editor::pumpInputQueue()
{
    glfwPollEvents();

    glfwGetCursorPos(mWindow, &mCurrentCursorPos.x, &mCurrentCursorPos.y);

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(static_cast<float>(mCurrentCursorPos.x),
						 static_cast<float>(mCurrentCursorPos.y));

	bool mousePressed[5];
	for(uint32_t i = 0; i < 5; ++i)
	{
		const auto pressed = glfwGetMouseButton(mWindow, i);

		mousePressed[i] = pressed == GLFW_PRESS;
	}
	memcpy(&io.MouseDown[0], &mousePressed[0], sizeof(bool) * 5);

	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(mWindow, &w, &h);
	glfwGetFramebufferSize(mWindow, &display_w, &display_h);
	io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w) / w, static_cast<float>(display_h) / h);
}


void Editor::drawMenuBar()
{
	if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if(ImGui::MenuItem("Load Scene"))
				{
                    mShowFileBrowser = true;
				}
				if(ImGui::MenuItem("Close current Scene"))
				{

                }
                if(ImGui::MenuItem("Add new mesh"))
                {
                    mShowFileBrowser = true;
                }

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				if(ImGui::MenuItem("About"))
                    mShowHelpMenu = true;

				ImGui::EndMenu();
			}

            // Radio buttons to select the mode.
            ImGui::BeginGroup();

                ImGui::RadioButton("Scene mode", &mMode, 0);
                ImGui::RadioButton("Shader graph mode", &mMode, 1);

            ImGui::EndGroup();

            ImGui::EndMainMenuBar();
	}
}


void Editor::drawAssistantWindow()
{
    const ImGuiIO& io = ImGui::GetIO();

    // TODO Using magic numbers for now, but will replace with proper scaling code later
    ImGui::SetNextWindowSize(ImVec2{io.DisplaySize.x / 4, io.DisplaySize.y - 20});
    ImGui::SetNextWindowPos(ImVec2{0, 20});

    if(ImGui::Begin("Assistant Editor"))
    {

       if(ImGui::BeginMenu("Add new task"))
       {
           ImGui::Indent(10.0f);

           drawPassContextMenu(PassType::DepthPre);
           drawPassContextMenu(PassType::GBuffer);
           drawPassContextMenu(PassType::SSAO);

           ImGui::EndMenu();
       }


        ImGui::End();
    }
}


void Editor::drawPassContextMenu(const PassType passType)
{
    if(ImGui::MenuItem(passToString(passType)))
    {
        mNodeEditor.addNode(static_cast<uint64_t>(passType));
    }
}
