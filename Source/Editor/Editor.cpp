#include "Editor/Editor.h"
#include "Engine/DefaultResourceSlots.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "imgui.h"
#include "ImGuizmo.h"

#include <atomic>


namespace
{

    std::shared_ptr<EditorNode> createPassNode(const uint64_t passType)
    {
        const NodeTypes pType = static_cast<NodeTypes>(passType);

        switch(pType)
        {
            case NodeTypes::DepthPre:
            {
                // Set the ID to 0 as the editor will set the real id.
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Depth Pre", passType);
                newNode->mOutputs.emplace_back(0, newNode, kGBufferDepth, PinType::Texture, PinKind::Output);
                return newNode;
            }

            case NodeTypes::GBuffer:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("GBuffer", passType);
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferNormals, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferAlbedo, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferMetalnessRoughness, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferDepth, PinType::Texture, PinKind::Output});
                return newNode;
            }

            case NodeTypes::SSAO:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("SSAO", passType);
                newNode->mInputs.push_back(Pin{0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input});
                newNode->mOutputs.push_back(Pin{0, newNode, kSSAO, PinType::Texture, PinKind::Output});
                return newNode;
            }

            case NodeTypes::SSAOImproved:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("SSAO Improved", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kSSAO, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::GBufferMaterial:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("GBufferMaterial", passType);
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferNormals, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferUV, PinType::Texture, PinKind::Output});
				newNode->mOutputs.push_back(Pin{0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferDepth, PinType::Texture, PinKind::Output});
                return newNode;
            }

            case NodeTypes::GBUfferMaterialPreDepth:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("GBufferMaterialPreDepth", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferNormals, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferUV, PinType::Texture, PinKind::Output});
				newNode->mOutputs.push_back(Pin{0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::GBufferPreDepth:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("GBufferPreDepth", passType);
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferNormals, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferAlbedo, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferMetalnessRoughness, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                newNode->mInputs.push_back(Pin{0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input});
                return newNode;
            }

			case NodeTypes::InplaceCombine:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("InplaceCombine", passType);
				newNode->mOutputs.push_back(Pin{ 0, newNode, "Combined", PinType::Texture, PinKind::Output });
				newNode->mInputs.push_back(Pin{ 0, newNode, "InOut", PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, "Factor", PinType::Texture, PinKind::Input });
				return newNode;
			}

			case NodeTypes::InplaceCombineSRGB:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("InplaceCombineSRGB", passType);
				newNode->mOutputs.push_back(Pin{ 0, newNode, "Combined", PinType::Texture, PinKind::Output });
				newNode->mInputs.push_back(Pin{ 0, newNode, "InOut", PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, "Factor", PinType::Texture, PinKind::Input });
				return newNode;
			}

			case NodeTypes::DeferredTextureBlinnPhongLighting:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("DeferredTextureBlinnPhongLighting", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferUV, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::DeferredTexturePBRIBL:
			{
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Deferred texuring IBL", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferUV, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::DeferredTextureAnalyticalPBRIBL:
			{
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Analytical deferred texuring IBL", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferUV, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

            case NodeTypes::DeferredPBRIBL:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("deferred IBL", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferAlbedo, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMetalnessRoughness, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
                return newNode;
            }

			case NodeTypes::ConvolveSkybox:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("ConvolveSkybox", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::Skybox:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Skybox", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::DFGGeneration:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("DFG", passType);
				newNode->mOutputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::ForwardIBL:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("ForwardIBL", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
				return newNode;
			}

            case NodeTypes::ForwardCombinedLighting:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("ForwardCombinedLighting", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                return newNode;
            }

			case NodeTypes::LightFroxelation:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("LightFroxelation", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::DeferredAnalyticalLighting:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Deferred Analytical Lighting", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferAlbedo, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMetalnessRoughness, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kAnalyticLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

            case NodeTypes::Shadow:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Shadow mapping", passType);
                newNode->mOutputs.push_back(Pin{ 0, newNode, kShadowMap, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::TAA:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("TAA", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kAnalyticLighting, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Input });
                return newNode;
            }
        }
    }

}


Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
	mCursorPosDelta{0.0, 0.0},
    mMode{1},
    mShowHelpMenu{false},
    mShowDebugTexturePicker{false},
    mCurrentDebugTexture(-1),
    mDebugTextureName(""),
    mInFreeFlyMode(false),
    mShowFileBrowser{false},
    mFileBrowser{"/"},
    mShowNodeEditor{false},
    mNodeEditor{"render graph editor", createPassNode},
    mRegisteredNodes{0},
	mSetupNeeded(true),
    mEngine{mWindow},
    mInProgressScene{"In construction"}
{
    ImGui::CreateContext();


    // Set up cursor scrolling.
    glfwSetWindowUserPointer(mWindow, this);
    auto curor_callback = [](GLFWwindow* window, double, double y)
    {
        Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
        editor->mouseScroll_callback(window, 0.0, y);
    };

    glfwSetScrollCallback(mWindow, curor_callback);
}


Editor::~Editor()
{
     ImGui::DestroyContext();
}


void Editor::run()
{
	int previousMode = 0;

    while(!glfwWindowShouldClose(mWindow))
    {
        pumpInputQueue();

		startFrame();

		if (previousMode != mMode)
			mEngine.clearRegisteredPasses();

        // Only draw the scene if we're in scene mode else draw the
        // pass/shader graphs.
        if(mMode == 0)
            renderScene();

		previousMode = mMode;
		renderOverlay();

		swap();

		endFrame();
	}
}

void Editor::renderScene()
{
	mNodeEditor.addPasses(mEngine);
}


void Editor::renderOverlay()
{
	// Dummy calls so that ImGui builds it's font texture
	if(mSetupNeeded)
	{
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	}
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    drawMenuBar();
    drawAssistantWindow();

    // We are on Pass/shader graph mode
    if(mMode == EditorMode::NodeEditor)
        mNodeEditor.draw();

    if(mShowFileBrowser)
    {
        auto optionalPath = mFileBrowser.render();

        if(optionalPath)
        {
            mShowFileBrowser = false;
			loadScene((*optionalPath).generic_u8string());
        }
    }

    // Set up the draw data/ also calls endFrame.
	ImGui::Render();

	mEngine.registerPass(PassType::Overlay);
	mEngine.registerPass(PassType::Composite);
}


void Editor::swap()
{
	mEngine.recordScene();
	mEngine.render();
	mEngine.swap();
}


void Editor::pumpInputQueue()
{
    glfwPollEvents();

    const double previousCursorX = mCurrentCursorPos.x;
    const double previousCursorY = mCurrentCursorPos.y;

    glfwGetCursorPos(mWindow, &mCurrentCursorPos.x, &mCurrentCursorPos.y);
    mCursorPosDelta.x = mCurrentCursorPos.x - previousCursorX;
    mCursorPosDelta.y = mCurrentCursorPos.y - previousCursorY;

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
    const double mouseDiff = mMouseScrollAmount.exchange(0.0);
    io.MouseWheel = static_cast<float>(mouseDiff);

	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(mWindow, &w, &h);
	glfwGetFramebufferSize(mWindow, &display_w, &display_h);
	io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    if (w > 0 && h > 0)
    {
        io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w) / w, static_cast<float>(display_h) / h);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        ImGuizmo::SetOrthographic(false);
    }

	if (mMode == EditorMode::SceneView) 
	{
		Camera& camera = mEngine.getCurrentSceneCamera();

		if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
			camera.moveForward(4.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
			camera.moveBackward(4.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
			camera.moveLeft(4.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
			camera.moveRight(4.5f);

        bool pressedEscape = glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (pressedEscape)
            mInFreeFlyMode = false;

        if (mInFreeFlyMode)
        {
            camera.rotatePitch(mCursorPosDelta.y);
            camera.rotateYaw(-mCursorPosDelta.x);
        }
	}
}


void Editor::mouseScroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    mMouseScrollAmount.store(yoffset);
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

    ImGui::SetNextWindowSize(ImVec2{io.DisplaySize.x / 4, io.DisplaySize.y - 20});
    ImGui::SetNextWindowPos(ImVec2{0, 20});

    if(ImGui::Begin("Assistant Editor"))
    {

       if(ImGui::TreeNode("Techniques"))
       {
           ImGui::Indent(10.0f);

           drawPassContextMenu(PassType::DepthPre);
           drawPassContextMenu(PassType::GBuffer);
           drawPassContextMenu(PassType::GBufferMaterial);
           drawPassContextMenu(PassType::GBufferPreDepth);
           drawPassContextMenu(PassType::GBUfferMaterialPreDepth);
           drawPassContextMenu(PassType::SSAO);
           drawPassContextMenu(PassType::SSAOImproved);
		   drawPassContextMenu(PassType::InplaceCombine);
		   drawPassContextMenu(PassType::InplaceCombineSRGB);
		   drawPassContextMenu(PassType::DeferredTextureBlinnPhongLighting);
		   drawPassContextMenu(PassType::DeferredTexturePBRIBL);
		   drawPassContextMenu(PassType::DeferredTextureAnalyticalPBRIBL);
           drawPassContextMenu(PassType::DeferredPBRIBL);
		   drawPassContextMenu(PassType::ForwardIBL);
           drawPassContextMenu(PassType::ForwardCombinedLighting);
		   drawPassContextMenu(PassType::Skybox);
		   drawPassContextMenu(PassType::ConvolveSkybox);
		   drawPassContextMenu(PassType::DFGGeneration);
		   drawPassContextMenu(PassType::LightFroxelation);
		   drawPassContextMenu(PassType::DeferredAnalyticalLighting);
           drawPassContextMenu(PassType::Shadow);
           drawPassContextMenu(PassType::TAA);

           ImGui::TreePop();
       }

       ImGui::Checkbox("Debug texture picker", &mShowDebugTexturePicker);
       ImGui::Checkbox("Camera free fly", &mInFreeFlyMode);

       drawLightMenu();
    }
    ImGui::End();

    std::vector<std::string> textures = mNodeEditor.getAvailableDebugTextures();
    if(mShowDebugTexturePicker)
    {
        drawDebugTexturePicker(textures);

        if(mCurrentDebugTexture != -1)
        {
            mDebugTextureName = textures[mCurrentDebugTexture];
        }
    }
    else
        mCurrentDebugTexture = -1;

    if(mCurrentDebugTexture != -1)
    {
        mEngine.enableDebugTexture(textures[mCurrentDebugTexture]);
    }
    else
    {
        mEngine.disableDebugTexture();
    }
}


void Editor::drawDebugTexturePicker(const std::vector<std::string>& textures)
{
    if(ImGui::Begin("Debug texture picker"))
    {

        ImGui::BeginGroup();

        for(int i = 0; i < textures.size(); ++i)
        {
            ImGui::RadioButton(textures[i].c_str(), &mCurrentDebugTexture, i);
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}


void Editor::drawLightMenu()
{
    const Camera& camera = mEngine.getScene().getCamera();
    const glm::mat4 viewMatrix = glm::lookAt(camera.getPosition(), camera.getPosition() + camera.getDirection(), float3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projectionMatrix = camera.getPerspectiveMatrix();

    if (ImGui::TreeNode("Lights"))
    {

        for (uint32_t i = 0; i < mLights.size(); ++i)
        {
            char lightName[15];
            sprintf(lightName, "Light #%d", i);
            if (ImGui::TreeNode(lightName))
            {
                EditorLight& light = mLights[i];
                Scene::Light& sceneLight = mEngine.getScene().getLight(light.mId);

                ImGui::Text("Position: X %f Y %f Z %f", light.mPosition.x, light.mPosition.y, light.mPosition.z);
                ImGui::Text("Direction: X %f Y %f Z %f", light.mDirection.x, light.mDirection.y, light.mDirection.z);
                ImGui::ColorEdit3("Colour", light.mColour, ImGuiColorEditFlags_InputRGB);

                drawGuizmo(light, viewMatrix, projectionMatrix);

                // Write back light updates to the scenes light buffer.
                sceneLight.mPosition = light.mPosition;
                sceneLight.mDirection = light.mDirection;
                sceneLight.mAlbedo = float4(light.mColour[0], light.mColour[1], light.mColour[2], 1.0f);

                ImGui::TreePop();
            }
        }

        if (ImGui::Button("Add Light"))
        {
            EditorLight newLight{};
            const size_t id = mEngine.getScene().addLight({ float4(0.0f, 0.0f, 0.0f, 1.0f),
                                                            float4(1.0f, 0.0f, 0.0f, 0.0f),
                                                            float4(1.0f, 1.0f, 1.0f, 1.0f),
                                                            2000.0f,
                                                            300.0f,
                                                            LightType::Point,
                                                            45.0f });
            newLight.mId = id;
            newLight.mType = LightType::Point;
            newLight.mPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
            newLight.mDirection = float4(1.0f, 0.0f, 0.0f, 1.0f);
            newLight.mColour[0] = 1.0f;
            newLight.mColour[1] = 1.0f;
            newLight.mColour[2] = 1.0f;
            newLight.mInfluence = 2000;

            mLights.push_back(newLight);
        }
        ImGui::TreePop();
    }
}


void Editor::drawGuizmo(EditorLight& light, const glm::mat4 &view, const glm::mat4 &proj)
{
    glm::mat4 lightTransformation = glm::translate(glm::mat4(1.0f), float3(light.mPosition));

    ImGuizmo::Enable(true);
    ImGuizmo::Manipulate(   glm::value_ptr(view),
                            glm::value_ptr(proj),
                            ImGuizmo::OPERATION::TRANSLATE,
                            ImGuizmo::MODE::WORLD,
                            glm::value_ptr(lightTransformation));

    light.mPosition = lightTransformation[3];
}


void Editor::drawPassContextMenu(const PassType passType)
{
    bool enabled = (static_cast<uint64_t>(passType) & mRegisteredNodes) > 0;
    const bool wasEnabled = enabled;
    ImGui::Checkbox(passToString(passType), &enabled);

    if (enabled != wasEnabled)
    {
        if (enabled)
        {
            mNodeEditor.addNode(static_cast<uint64_t>(passType));
            mRegisteredNodes |= static_cast<uint64_t>(passType);
        }
        else
        {
            mNodeEditor.removeNode(static_cast<uint64_t>(passType));
            mRegisteredNodes &= ~static_cast<uint64_t>(passType);
        }
    }
}


void Editor::loadScene(const std::string& scene)
{
	// Default Editor skybox.
    std::array<std::string, 6> skybox{	"./Assets/Skybox.png",
                                        "./Assets/Skybox.png",
                                        "./Assets/Skybox.png",
                                        "./Assets/Skybox.png",
                                        "./Assets/Skybox.png",
                                        "./Assets/Skybox.png" };

	Scene testScene(scene);
	testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, &mEngine);
	testScene.loadSkybox(skybox, &mEngine);
	testScene.uploadData(&mEngine);
	testScene.computeBounds(MeshType::Static);
	testScene.computeBounds(MeshType::Dynamic);

	mEngine.setScene(testScene);
}
