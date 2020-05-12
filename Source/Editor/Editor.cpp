#include "Editor/Editor.h"
#include "Engine/DefaultResourceSlots.hpp"
#include "Engine/TextureUtil.hpp"

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
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferDiffuse, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferSpecularRoughness, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferEmissiveOcclusion, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferDepth, PinType::Texture, PinKind::Output});
                return newNode;
            }

            case NodeTypes::SSAO:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("SSAO", passType);
                newNode->mInputs.push_back(Pin{0, newNode, kLinearDepth, PinType::Texture, PinKind::Input});
                newNode->mOutputs.push_back(Pin{0, newNode, kSSAO, PinType::Texture, PinKind::Output});
                return newNode;
            }

            case NodeTypes::SSAOImproved:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("SSAO Improved", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kLinearDepth, PinType::Texture, PinKind::Input });
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
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferDiffuse, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{0, newNode, kGBufferSpecularRoughness, PinType::Texture, PinKind::Output});
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferEmissiveOcclusion, PinType::Texture, PinKind::Output });
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

            case NodeTypes::DeferredPBRIBL:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("deferred IBL", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDiffuse, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferSpecularRoughness, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferEmissiveOcclusion, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedDiffuseSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSpecularSkyBox, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
                return newNode;
            }

			case NodeTypes::ConvolveSkybox:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("ConvolveSkybox", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kSkyBox, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kConvolvedDiffuseSkyBox, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kConvolvedSpecularSkyBox, PinType::Texture, PinKind::Output });
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
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedDiffuseSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSpecularSkyBox, PinType::Texture, PinKind::Input });
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
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedDiffuseSkyBox, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kConvolvedSpecularSkyBox, PinType::Texture, PinKind::Input });                newNode->mInputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Output });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Output });
                return newNode;
            }

			case NodeTypes::LightFroxelation:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("LightFroxelation", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kLinearDepth, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Output });
				return newNode;
			}

			case NodeTypes::DeferredAnalyticalLighting:
			{
				std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Deferred Analytical Lighting", passType);
				newNode->mInputs.push_back(Pin{ 0, newNode, kDFGLUT, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDiffuse, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferSpecularRoughness, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
				newNode->mInputs.push_back(Pin{ 0, newNode, kActiveFroxels, PinType::Texture, PinKind::Input });
				newNode->mOutputs.push_back(Pin{ 0, newNode, kAnalyticLighting, PinType::Texture, PinKind::Output });
				return newNode;
			}

            case NodeTypes::Shadow:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Shadow mapping", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kShadowMap, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::CascadingShadow:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Cascade shadow mapping", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kShadowMap, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::TAA:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("TAA", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferVelocity, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kLinearDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kAnalyticLighting, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Input });
                return newNode;
            }

            case NodeTypes::LineariseDepth:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Linearise depth", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kLinearDepth, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::SSR:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Screen space reflection", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kLinearDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferNormals, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferSpecularRoughness, PinType::Texture, PinKind::Input });
                newNode->mOutputs.push_back(Pin{ 0, newNode, kReflectionMap, PinType::Texture, PinKind::Output });
                return newNode;
            }

            case NodeTypes::Transparent:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Screen space reflection", passType);
                newNode->mInputs.push_back(Pin{ 0, newNode, kGBufferDepth, PinType::Texture, PinKind::Input });
                newNode->mInputs.push_back(Pin{ 0, newNode, kGlobalLighting, PinType::Texture, PinKind::Input });
                return newNode;
            }

            case NodeTypes::Animation:
            {
                std::shared_ptr<EditorNode> newNode = std::make_shared<PassNode>("Skinning", passType);
                return newNode;
            }
        }
    }

}


Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
	mCursorPosDelta{0.0, 0.0},
    mMode{EditorMode::NodeEditor},
    mShowHelpMenu{false},
    mShowDebugTexturePicker{false},
    mCurrentDebugTexture(-1),
    mDebugTextureName(""),
    mRecompileGraph(false),
    mCameraInfo{false, 1.0f, 0.1f, 2000.0f, 90.0f},
    mShowFileBrowser{false},
    mFileBrowser{"scene", "/"},
    mShowNodeEditor{false},
    mNodeEditor{"render graph editor", createPassNode},
    mRegisteredNodes{0},
	mSetupNeeded(true),
    mEngine{mWindow},
    mSceneInstanceIDs{},
    mInProgressScene{new Scene("Default")},
    mStaticMeshEntries{},
    mShowMeshFileBrowser{false},
    mShowMaterialDialog{false},
    mMaterialDialog{},
    mMaterialNames{},
    mCurrentMaterialIndex{0},
    mMeshToInstance{0},
    mShowAddInstanceDialog{false},
    mResetSceneAtEndOfFrame{false},
    mPublishedScene{false},
    mLightOperationMode{ImGuizmo::OPERATION::TRANSLATE},
    mEditShadowingLight{false}
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

    auto text_callback = [](GLFWwindow* window, unsigned int codePoint)
    {
        Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
        editor->text_callback(window, codePoint);
    };
    glfwSetCharCallback(mWindow, text_callback);

    // Zero the scratch buffer.
    memset(mMeshInstanceScratchBuffer, 0, 32);
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

        if (mRecompileGraph)
        {
			mEngine.clearRegisteredPasses();
            mRecompileGraph = false;
        }

        // Only draw the scene if we're in scene mode else draw the
        // pass/shader graphs.
        if((mMode == EditorMode::SceneView) && mPublishedScene)
            renderScene();

        previousMode = mMode;
        renderOverlay();

        mRecompileGraph = mRecompileGraph || previousMode != mMode;

		swap();

		endFrame();
	}
}

void Editor::renderScene()
{
    mInProgressScene->updateMaterialsLastAccess();
    mEngine.registerPass(PassType::DebugAABB);
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
			loadScene((*optionalPath).string());
        }
    }

    if(mShowMeshFileBrowser)
    {
        auto optionalPath = mFileBrowser.render();

        if(optionalPath)
        {
            mShowMeshFileBrowser = false;

            StaticMesh mesh(optionalPath->string(), VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);
            SceneID id = mInProgressScene->addMesh(std::move(mesh), MeshType::Dynamic);

            mStaticMeshEntries.push_back({id, optionalPath->filename().string()});

            mInProgressScene->uploadData(&mEngine);
        }
    }

    if(mShowMaterialDialog)
    {
        bool created = mMaterialDialog.render();

        if(created)
        {
            mShowMaterialDialog = false;
            addMaterial();
            ImGui::GetIO().ClearInputCharacters();
        }
    }

    if(mShowAddInstanceDialog)
        drawAddInstanceDialog();

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

    if(mResetSceneAtEndOfFrame)
    {
        delete mInProgressScene;
        mInProgressScene = new Scene("InProgress");

        mEngine.setScene(nullptr);
        mResetSceneAtEndOfFrame = false;
        mPublishedScene = false;
        mMode = EditorMode::NodeEditor;
        mRecompileGraph = true;

        mSceneInstanceIDs.clear();
        mStaticMeshEntries.clear();
        mLights.clear();
    }
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
            camera.moveForward(mCameraInfo.mCameraSpeed);
		if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
            camera.moveBackward(mCameraInfo.mCameraSpeed);
		if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
            camera.moveLeft(mCameraInfo.mCameraSpeed);
		if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
            camera.moveRight(mCameraInfo.mCameraSpeed);

        if (glfwGetKey(mWindow, GLFW_KEY_R) == GLFW_PRESS)
            mLightOperationMode = ImGuizmo::OPERATION::ROTATE;
        else if (glfwGetKey(mWindow, GLFW_KEY_T) == GLFW_PRESS)
            mLightOperationMode = ImGuizmo::OPERATION::TRANSLATE;
        else if(glfwGetKey(mWindow, GLFW_KEY_Y) == GLFW_PRESS)
            mLightOperationMode = ImGuizmo::OPERATION::SCALE;

        bool pressedEscape = glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (pressedEscape)
        {
            mCameraInfo.mInFreeFlyMode = false;
            glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mCameraInfo.mInFreeFlyMode)
        {
            camera.rotatePitch(mCursorPosDelta.y);
            camera.rotateWorldUp(-mCursorPosDelta.x);
        }
	}
    else // Add key presses for text input.
    {

    }
}


void Editor::mouseScroll_callback(GLFWwindow*, double, double yoffset)
{
    mMouseScrollAmount.store(yoffset);
}


void Editor::text_callback(GLFWwindow*, unsigned int codePoint)
{
    ImGui::GetIO().AddInputCharacter(codePoint);
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
                    mResetSceneAtEndOfFrame = true;
                }
                if(ImGui::MenuItem("Load mesh"))
                {
                    mShowMeshFileBrowser = true;
                }
                if(ImGui::MenuItem("Load material"))
                {
                    mShowMaterialDialog = true;
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

            if(!mPublishedScene && ImGui::Button("publish scene"))
            {
                mEngine.setScene(mInProgressScene);

                std::array<std::string, 6> skybox{	"./Assets/skybox/px.png",
                                                    "./Assets/skybox/nx.png",
                                                    "./Assets/skybox/py.png",
                                                    "./Assets/skybox/ny.png",
                                                    "./Assets/skybox/pz.png",
                                                    "./Assets/skybox/nz.png" };
                mInProgressScene->loadSkybox(skybox, &mEngine);

                mPublishedScene = true;
            }

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
           drawPassContextMenu(PassType::DeferredPBRIBL);
		   drawPassContextMenu(PassType::ForwardIBL);
           drawPassContextMenu(PassType::ForwardCombinedLighting);
		   drawPassContextMenu(PassType::Skybox);
		   drawPassContextMenu(PassType::ConvolveSkybox);
		   drawPassContextMenu(PassType::DFGGeneration);
		   drawPassContextMenu(PassType::LightFroxelation);
		   drawPassContextMenu(PassType::DeferredAnalyticalLighting);
           drawPassContextMenu(PassType::Shadow);
           drawPassContextMenu(PassType::CascadingShadow);
           drawPassContextMenu(PassType::TAA);
           drawPassContextMenu(PassType::LineariseDepth);
           drawPassContextMenu(PassType::SSR);
           drawPassContextMenu(PassType::Transparent);
           drawPassContextMenu(PassType::Animation);

           ImGui::TreePop();
       }

       if(ImGui::TreeNode("Static Meshes"))
       {

           for(const StaticMeshEntry& entry : mStaticMeshEntries)
           {
               ImGui::TextUnformatted(entry.mName.c_str());
               ImGui::SameLine();
               if(ImGui::Button("Add instance"))
               {
                    mShowAddInstanceDialog = true;
               }
           }

           ImGui::TreePop();
       }

       if(ImGui::TreeNode("Mesh instances"))
       {

           const Camera& camera = mInProgressScene->getCamera();
           const float4x4 viewMatrix = glm::lookAt(camera.getPosition(), camera.getPosition() + camera.getDirection(), float3(0.0f, 1.0f, 0.0f));
           const float4x4 projectionMatrix = camera.getPerspectiveMatrix();

           for(InstanceID ID : mSceneInstanceIDs)
           {
                MeshInstance* instance = mInProgressScene->getMeshInstance(ID);

                if(ImGui::TreeNode(instance->getName().c_str()))
                {
                    uint32_t instanceFlags = instance->getInstanceFlags();
                    bool draw = instanceFlags & InstanceFlags::Draw;
                    ImGui::Checkbox("Draw", &draw);

                    bool wireFrame = instanceFlags & InstanceFlags::DrawWireFrame;
                    ImGui::Checkbox("WireFrame", &wireFrame);

                    bool aabb = instanceFlags & InstanceFlags::DrawAABB;
                    ImGui::Checkbox("AABB", &aabb);

                    bool guizmo = instanceFlags & InstanceFlags::DrawGuizmo;
                    ImGui::Checkbox("Guizmo", &guizmo);

                    if(instance->mMesh->hasAnimations())
                    {
                        const auto animations = instance->mMesh->getAllAnimations();
                        ImGui::TextUnformatted("Play animation");
                        uint32_t i = 0;
                        for(const auto& [name, _] : animations)
                        {
                            char nameBuffer[12];
                            sprintf(nameBuffer, "Animation %d\n", i);
                            if(ImGui::Button(nameBuffer))
                            {
                                mEngine.startAnimation(ID, name);
                            }
                            ++i;
                        }
                    }

                    if(guizmo)
                    {
                        float4x4 transMatrix = instance->getTransMatrix();
                        drawGuizmo(transMatrix, viewMatrix, projectionMatrix, mLightOperationMode);

                        instance->setTransMatrix(transMatrix);
                    }

                    uint32_t newInstanceFlags = 0;
                    newInstanceFlags |= draw ? InstanceFlags::Draw : 0;
                    newInstanceFlags |= wireFrame ? InstanceFlags::DrawWireFrame : 0;
                    newInstanceFlags |= aabb ? InstanceFlags::DrawAABB : 0;
                    newInstanceFlags |= guizmo ? InstanceFlags::DrawGuizmo : 0;

                    instance->setInstanceFlags(newInstanceFlags);

                    ImGui::TreePop();
                }
           }

           ImGui::TreePop();
       }

       if(ImGui::TreeNode("Materials"))
       {
           for(const auto& materialName : mMaterialNames)
           {
               ImGui::TextUnformatted(materialName.c_str());
           }

           ImGui::TreePop();
       }

       ImGui::Checkbox("Debug texture picker", &mShowDebugTexturePicker);

       if(ImGui::TreeNode("Camera settings"))
       {
           const bool previouseFlyMode = mCameraInfo.mInFreeFlyMode;
           ImGui::Checkbox("Camera free fly", &mCameraInfo.mInFreeFlyMode);
           ImGui::SliderFloat("Camera speed", &mCameraInfo.mCameraSpeed, 0.01f, 10.0f);
           ImGui::SliderFloat("near plane", &mCameraInfo.mNearPlaneDistance, 0.01f, 10.0f);
           ImGui::SliderFloat("far plane", &mCameraInfo.mFarPlaneDistance, 50.0f, 2000.0f);
           ImGui::SliderFloat("field of view", &mCameraInfo.mFOV, 45.0f, 180.0f);

           if(mCameraInfo.mInFreeFlyMode && !previouseFlyMode)
               glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

           ImGui::TreePop();

           // update the current scene camera.
           Camera& camera = mInProgressScene->getCamera();
           camera.setNearPlane(mCameraInfo.mNearPlaneDistance);
           camera.setFarPlane(mCameraInfo.mFarPlaneDistance);
           camera.setFOVDegrees(mCameraInfo.mFOV);
        }

       drawLightMenu();
    }
    ImGui::End();

    std::vector<std::string> textures = mNodeEditor.getAvailableDebugTextures();
    if(mShowDebugTexturePicker)
    {
        drawDebugTexturePicker(textures);

        if(mCurrentDebugTexture != -1)
        {
            std::string previousDebugTexture = mDebugTextureName;
            mDebugTextureName = textures[mCurrentDebugTexture];

            if(mDebugTextureName != previousDebugTexture)
                mRecompileGraph = true;
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

        for(size_t i = 0; i < textures.size(); ++i)
        {
            ImGui::RadioButton(textures[i].c_str(), &mCurrentDebugTexture, i);
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}


void Editor::drawLightMenu()
{
    const Camera& camera = mInProgressScene->getCamera();
    const float4x4 viewMatrix = glm::lookAt(camera.getPosition(), camera.getPosition() + camera.getDirection(), float3(0.0f, 1.0f, 0.0f));
    const float4x4 projectionMatrix = camera.getPerspectiveMatrix();

    if (ImGui::TreeNode("Lights"))
    {

        for (uint32_t i = 0; i < mLights.size(); ++i)
        {
            char lightName[15];
            sprintf(lightName, "Light #%d", i);
            if (ImGui::TreeNode(lightName))
            {
                EditorLight& light = mLights[i];

                ImGui::InputFloat3("Position", &light.mPosition[0]);
                ImGui::InputFloat3("Direction", &light.mDirection[0]);
                ImGui::ColorEdit3("Colour", light.mColour, ImGuiColorEditFlags_InputRGB);
                ImGui::SliderFloat("influence radius", &light.mRadius, 0.1f, 500.0f);
                ImGui::SliderFloat("intensity", &light.mIntensity, 0.1f, 50.0f);

                drawGuizmo(light, viewMatrix, projectionMatrix, mLightOperationMode);

                // Write back light updates to the scenes light buffer.
                Scene::Light& sceneLight = mInProgressScene->getLight(light.mId);
                sceneLight.mPosition = light.mPosition;
                sceneLight.mDirection = light.mDirection;
                sceneLight.mAlbedo = float4(light.mColour[0], light.mColour[1], light.mColour[2], 1.0f);
                sceneLight.mRadius = light.mRadius;
                sceneLight.mIntensity = light.mIntensity;

                ImGui::TreePop();
            }
        }

        if (ImGui::Button("Add point light"))
        {
            EditorLight newLight{};
            const size_t id = mInProgressScene->addLight(Scene::Light::pointLight(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(1.0f), 20.0f, 300.0f));
            newLight.mId = id;
            newLight.mType = LightType::Point;
            newLight.mPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
            newLight.mDirection = float4(1.0f, 0.0f, 0.0f, 1.0f);
            newLight.mColour[0] = 1.0f;
            newLight.mColour[1] = 1.0f;
            newLight.mColour[2] = 1.0f;
            newLight.mIntensity = 20.0f;
            newLight.mRadius = 300.0f;

            mLights.push_back(newLight);
        }

        if (ImGui::Button("Add spot light"))
        {
            EditorLight newLight{};
            const size_t id = mInProgressScene->addLight(Scene::Light::spotLight(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 0.0f), float4(1.0f), 20.0f, 300.0f, 45.0f));
            newLight.mId = id;
            newLight.mType = LightType::Spot;
            newLight.mPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
            newLight.mDirection = float4(1.0f, 0.0f, 0.0f, 1.0f);
            newLight.mColour[0] = 1.0f;
            newLight.mColour[1] = 1.0f;
            newLight.mColour[2] = 1.0f;
            newLight.mIntensity = 20.0f;
            newLight.mRadius = 300.0f;

            mLights.push_back(newLight);
        }

        if (ImGui::Button("Add area light"))
        {
            EditorLight newLight{};
            const size_t id = mInProgressScene->addLight(Scene::Light::areaLight(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0), float4(0.0f, 1.0f, 0.0f, 0.0f), float4(1.0f), 20.0f, 300.0f, 50.0f));
            newLight.mId = id;
            newLight.mType = LightType::Area;
            newLight.mPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
            newLight.mDirection = float4(1.0f, 0.0f, 0.0f, 1.0f);
            newLight.mColour[0] = 1.0f;
            newLight.mColour[1] = 1.0f;
            newLight.mColour[2] = 1.0f;
            newLight.mIntensity = 20.0f;
            newLight.mRadius = 300.0f;

            mLights.push_back(newLight);
        }

        // CUrrentlt not implemented.
        /*if (ImGui::Button("Add strip light"))
        {
            EditorLight newLight{};
            const size_t id = mInProgressScene.addLight(Scene::Light::stripLight(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(0.0f, 1.0f, 0.0f, 0.0f), float4(1.0f), 20.0f, 300.0f, 50.0f));
            newLight.mId = id;
            newLight.mType = LightType::Strip;
            newLight.mPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
            newLight.mDirection = float4(1.0f, 0.0f, 0.0f, 1.0f);
            newLight.mColour[0] = 1.0f;
            newLight.mColour[1] = 1.0f;
            newLight.mColour[2] = 1.0f;
            newLight.mInfluence = 20;

            mLights.push_back(newLight);
        }*/

        Scene::ShadowingLight& shadowingLight = mInProgressScene->getShadowingLight();
        Scene::ShadowCascades& cascades = mInProgressScene->getShadowCascades();
        ImGui::Text("Shadowing light position  X: %f Y: %f, Z: %f", shadowingLight.mPosition.x, shadowingLight.mPosition.y, shadowingLight.mPosition.z);
        ImGui::Text("Shadowing light direction X: %f Y: %f, Z: %f", shadowingLight.mDirection.x, shadowingLight.mDirection.y, shadowingLight.mDirection.z);
        ImGui::DragFloat3("Cascades", &cascades.mNearEnd, 5.0f, 10.0f, 2000.0f);
        ImGui::Checkbox("shadow ligth ImGuizmo", &mEditShadowingLight);
        if(mEditShadowingLight)
        {
            float4x4 lightTransformation(1.0f);

            if (mEditShadowingLight == ImGuizmo::OPERATION::TRANSLATE)
                lightTransformation = glm::translate(lightTransformation, float3(shadowingLight.mPosition.x, shadowingLight.mPosition.y, shadowingLight.mPosition.z));

            drawGuizmo(lightTransformation, viewMatrix, projectionMatrix, mLightOperationMode);

            if(mLightOperationMode == ImGuizmo::OPERATION::TRANSLATE)
                shadowingLight.mPosition = lightTransformation[3];
            else if (mLightOperationMode == ImGuizmo::OPERATION::ROTATE)
            {
                shadowingLight.mDirection = lightTransformation * shadowingLight.mDirection;
                shadowingLight.mUp = lightTransformation * shadowingLight.mUp;
            }

            mInProgressScene->setShadowingLight(shadowingLight.mPosition, shadowingLight.mDirection, shadowingLight.mUp );
        }


        ImGui::TreePop();
    }
}


void Editor::drawGuizmo(EditorLight& light, const float4x4 &view, const float4x4 &proj, const ImGuizmo::OPERATION op)
{
    float4x4 lightTransformation(1.0f);

    if (op == ImGuizmo::OPERATION::TRANSLATE)
        lightTransformation = glm::translate(lightTransformation, float3(light.mPosition.x, light.mPosition.y, light.mPosition.z));

    ImGuizmo::Enable(true);
    ImGuizmo::Manipulate(   glm::value_ptr(view),
                            glm::value_ptr(proj),
                            op,
                            ImGuizmo::MODE::WORLD,
                            glm::value_ptr(lightTransformation));

    if(op == ImGuizmo::OPERATION::TRANSLATE)
        light.mPosition = lightTransformation[3];
    else if (op == ImGuizmo::OPERATION::ROTATE)
    {
        light.mDirection = lightTransformation * light.mDirection;
        light.mUp = lightTransformation * light.mUp;
    }
}


void Editor::drawGuizmo(float4x4& world, const float4x4& view, const float4x4& proj, const ImGuizmo::OPERATION op)
{
    ImGuizmo::Manipulate(   glm::value_ptr(view),
                            glm::value_ptr(proj),
                            op,
                            ImGuizmo::MODE::WORLD,
                            glm::value_ptr(world));
}


void Editor::addMaterial()
{
    const uint32_t materialFlags = mMaterialDialog.getMaterialFlags();
    Scene::MaterialPaths newMaterial{};
    newMaterial.mMaterialTypes = materialFlags;

    if(materialFlags & MaterialType::Albedo || materialFlags & MaterialType::Diffuse)
    {
        newMaterial.mAlbedoorDiffusePath = mMaterialDialog.getPath(AlbedoDiffusePath);
    }

    if(materialFlags & MaterialType::Normals)
    {
        newMaterial.mNormalsPath = mMaterialDialog.getPath(NormalsPath);
    }

    if(materialFlags & MaterialType::Roughness || materialFlags & MaterialType::Gloss)
    {
        newMaterial.mRoughnessOrGlossPath = mMaterialDialog.getPath(RoughnessGlossPath);
    }

    if(materialFlags & MaterialType::Metalness || materialFlags & MaterialType::Specular)
    {
        newMaterial.mMetalnessOrSpecularPath = mMaterialDialog.getPath(MetalnessSpecularPath);
    }

    if (materialFlags & MaterialType::Emisive)
    {
        newMaterial.mEmissivePath = mMaterialDialog.getPath(EmissivePath);
    }

    if (materialFlags & MaterialType::AmbientOcclusion)
    {
        newMaterial.mEmissivePath = mMaterialDialog.getPath(AOPath);
    }

    if (materialFlags & MaterialType::CombinedMetalnessRoughness)
    {
        newMaterial.mRoughnessOrGlossPath = mMaterialDialog.getPath(RoughnessGlossPath);
    }

    if (materialFlags & MaterialType::CombinedSpecularGloss)
    {
        newMaterial.mMetalnessOrSpecularPath = mMaterialDialog.getPath(MetalnessSpecularPath);
    }

    mInProgressScene->addMaterial(newMaterial, &mEngine);

    mMaterialNames.push_back(mMaterialDialog.getMaterialName());
    mMaterialDialog.reset();
}


void Editor::drawAddInstanceDialog()
{
    if(ImGui::Begin("Add Instance"))
    {
        if(ImGui::BeginCombo("Material", mMaterialNames[mCurrentMaterialIndex].c_str()))
        {
            for(uint32_t i = 0; i < mMaterialNames.size(); ++i)
            {
                if(ImGui::Selectable(mMaterialNames[i].c_str()), mCurrentMaterialIndex == i)
                    mCurrentMaterialIndex = i;
            }

            ImGui::EndCombo();
        }

        ImGui::InputText("instance name:", mMeshInstanceScratchBuffer, 32);

        if(ImGui::Button("Create"))
        {
            mShowAddInstanceDialog = false;
            const Scene::Material& mat = mInProgressScene->getMaterialDescriptions()[mCurrentMaterialIndex];
            const InstanceID id = mInProgressScene->addMeshInstance(mMeshToInstance, float4x4(1.0f), mat.mMaterialOffset, mat.mMaterialTypes, mMeshInstanceScratchBuffer);
            mSceneInstanceIDs.push_back(id);

            mInProgressScene->computeBounds(MeshType::Static);
            mInProgressScene->computeBounds(MeshType::Dynamic);

            ImGui::GetIO().ClearInputCharacters();
            memset(mMeshInstanceScratchBuffer, 0, 32);
        }
    }
    ImGui::End();
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
    mInProgressScene->setPath(scene);
    mSceneInstanceIDs = mInProgressScene->loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates, &mEngine);
    mInProgressScene->uploadData(&mEngine);
    mInProgressScene->computeBounds(MeshType::Static);
    mInProgressScene->computeBounds(MeshType::Dynamic);

    std::array<std::string, 6> skybox{	"./Assets/skybox/px.png",
                                        "./Assets/skybox/nx.png",
                                        "./Assets/skybox/py.png",
                                        "./Assets/skybox/ny.png",
                                        "./Assets/skybox/pz.png",
                                        "./Assets/skybox/nz.png" };
    mInProgressScene->loadSkybox(skybox, &mEngine);

    mEngine.setScene(mInProgressScene);

    mPublishedScene = true; // Scene has been published to engine.
}
