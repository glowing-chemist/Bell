
#include "Editor/Editor.h"

#include "imgui.h"

Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
    mMode{0},
    mShowHelpMenu{false},
    mShowFileBrowser{false},
    mFileBrowser{"/"},
    mEngine{mWindow},
	mHasUploadedFonts(false),
	mOverlayFontTexture(mEngine.createImage(1, 1, 1, vk::Format::eR8Srgb, vk::ImageUsageFlagBits::eSampled, "Font Texture")),
	mOverlayTranslationUBO(mEngine.createBuffer(16, 16, vk::BufferUsageFlagBits::eUniformBuffer, "Transformations")),
	mFontsSampler(SamplerType::Linear),
    mInProgressScene{"In construction"}
{
    ImGui::CreateContext();

	mFontsSampler.setAddressModeU(AddressMode::Repeat);
	mFontsSampler.setAddressModeV(AddressMode::Repeat);
	mFontsSampler.setAddressModeW(AddressMode::Repeat);

	// Set this to nullptr until I can figure out why it casues us not to draw anything.
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
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

		Image fontTexture = mEngine.createImage(width,
												height,
												1,
												vk::Format::eR8G8B8A8Unorm,
												vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
												"Overlay Fonts");
		mOverlayFontTexture.swap(fontTexture);

		mOverlayFontTexture.setContents(pixels, width, height, 1);

		auto imageView = mOverlayFontTexture.createImageView(vk::Format::eR8G8B8A8Unorm,
															 vk::ImageViewType::e2D,
															 0, 1, 0, 1);

		mOverlayFontTexture.setCurrentImageView(imageView);

		mHasUploadedFonts = true;
	}

	ImGui::NewFrame();

	addMenuBar();

    if(mShowFileBrowser)
    {
        ImGui::SetNextWindowPos(ImVec2(800, 450));
        auto optionalPath = mFileBrowser.render();

        if(optionalPath)
            mShowFileBrowser = false;
    }

	// Set up the draw data.
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
	mEngine.setImageInScene("OverlayTexture", mOverlayFontTexture);
	mEngine.setImageInScene("FrameBuffer", mEngine.getSwapChainImage());
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
	for(uint i = 0; i < 5; ++i)
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


void Editor::addMenuBar()
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
