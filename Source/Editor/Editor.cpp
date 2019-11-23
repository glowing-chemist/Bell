
#include "Editor/Editor.h"
#include "Engine/DefaultResourceSlots.hpp"

#include "imgui.h"

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
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferSpecular, PinType::Texture, PinKind::Output});
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

            case NodeTypes::GBufferMaterial:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("GBufferMaterial", passType);
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferNormals, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferUV, PinType::Texture, PinKind::Output});
				newNode->mOutputs.push_back(Pin{0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Output });
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
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferSpecular, PinType::Texture, PinKind::Output});
                newNode->mInputs.push_back(Pin{0, newNode,  kGBufferDepth, PinType::Texture, PinKind::Input});
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
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("PBR IBL", passType);
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
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Analytical IBL", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferUV, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferMaterialID, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::ConvolveSkybox:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("ConvolveSkybox", passType);
				newNode->mOutputs.push_back(Pin{ 0, newNode, kConvolvedSkyBox, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::Skybox:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Skybox", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
				return newNode;
			}

			case NodeTypes::DFGGeneration:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("DFG", passType);
				newNode->mOutputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Output });
				return newNode;
			}
        }
    }

}


Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
    mMode{1},
    mShowHelpMenu{false},
    mShowFileBrowser{false},
    mFileBrowser{"/"},
    mShowNodeEditor{false},
    mNodeEditor{"render graph editor", createPassNode},
	mSetupNeeded(true),
    mEngine{mWindow},
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

	if (mMode == EditorMode::SceneView) 
	{
		Camera& camera = mEngine.getCurrentSceneCamera();

		if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
			camera.moveForward(0.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
			camera.moveBackward(0.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
			camera.moveLeft(0.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
			camera.moveRight(0.5f);
		if (glfwGetKey(mWindow, GLFW_KEY_Q) == GLFW_PRESS)
			camera.rotateYaw(1.0f);
		if (glfwGetKey(mWindow, GLFW_KEY_E) == GLFW_PRESS)
			camera.rotateYaw(-1.0f);
		if (glfwGetKey(mWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
			camera.moveUp((0.5f));
		if (glfwGetKey(mWindow, GLFW_KEY_C) == GLFW_PRESS)
			camera.moveDown(0.5f);
	}
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
           drawPassContextMenu(PassType::GBufferMaterial);
           drawPassContextMenu(PassType::GBufferPreDepth);
           drawPassContextMenu(PassType::GBUfferMaterialPreDepth);
           drawPassContextMenu(PassType::SSAO);
		   drawPassContextMenu(PassType::InplaceCombine);
		   drawPassContextMenu(PassType::InplaceCombineSRGB);
		   drawPassContextMenu(PassType::DeferredTextureBlinnPhongLighting);
		   drawPassContextMenu(PassType::DeferredTexturePBRIBL);
		   drawPassContextMenu(PassType::DeferredTextureAnalyticalPBRIBL);
		   drawPassContextMenu(PassType::Skybox);
		   drawPassContextMenu(PassType::ConvolveSkybox);
		   drawPassContextMenu(PassType::DFGGeneration);

           ImGui::EndMenu();
       }

       if(ImGui::BeginMenu("Add new resource"))
       {
           if(ImGui::MenuItem("Texture"))
           {
               std::shared_ptr<EditorNode> newNode = std::make_shared<ResourceNode>("Texture", std::numeric_limits<uint64_t>::max());
               newNode->mOutputs.push_back(Pin{0, newNode, "Sample", PinType::Texture, PinKind::Output});

               mNodeEditor.addNode(newNode);
           }

           if(ImGui::MenuItem("Buffer"))
           {
               std::shared_ptr<EditorNode> newNode = std::make_shared<ResourceNode>("Buffer", std::numeric_limits<uint64_t>::max() - 1);
               newNode->mOutputs.push_back(Pin{0, newNode, "Load", PinType::Buffer, PinKind::Output});

               mNodeEditor.addNode(newNode);
           }

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


void Editor::loadScene(const std::string& scene)
{
	// Default Editor skybox.
	std::array<std::string, 6> skybox{	"./skybox/peaks_ft.tga",
										"./skybox/peaks_bk.tga", 
										"./skybox/peaks_up.tga", 
										"./skybox/peaks_dn.tga", 
										"./skybox/peaks_rt.tga", 
										"./skybox/peaks_lf.tga" };

	Scene testScene(scene);
	testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, &mEngine);
	testScene.loadSkybox(skybox, &mEngine);

	mEngine.setScene(testScene);
}
