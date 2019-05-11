
#include "Editor/Editor.h"

#include "imgui.h"

Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
    mEngine{mWindow},
	mHasUploadedFonts(false),
	mOverlayFontTexture(mEngine.createImage(1, 1, 1, vk::Format::eR8Srgb, vk::ImageUsageFlagBits::eSampled)),
	mFontsSampler(SamplerType::Linear),
    mInProgressScene{"In construction"}
{
    ImGui::CreateContext();
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
	if(!mHasUploadedFonts)
	{
		ImGuiIO& io = ImGui::GetIO();

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		size_t upload_size = width*height;

		mOverlayFontTexture = mEngine.createImage(width,
												  height,
												  1,
												  vk::Format::eR32G32B32A32Sfloat,
												  vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
												  "Overlay Fonts");

		mOverlayFontTexture.setContents(pixels, width, height, 1);

		mHasUploadedFonts = true;
	}

	bool demoWindow = true;
	ImGui::ShowDemoWindow(&demoWindow);
	// Set up the draw data.
	ImGui::Render();

	mEngine.recordOverlay(ImGui::GetDrawData());


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
}
