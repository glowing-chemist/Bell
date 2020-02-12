#include <GLFW/glfw3.h>
#include <vector>
#include <string>

#include <imgui.h>

#include "Engine/Engine.hpp"

struct ImGuiOptions
{
    int mToggleIndex = 0;
    bool mDefered = true;
    bool mForward = false;
    bool mShowLights = true;
};

static ImGuiOptions graphicsOptions;

bool renderMenu(GLFWwindow* win, const Camera& cam)
{
	double cursorPosx, cursorPosy;
	glfwGetCursorPos(win, &cursorPosx, &cursorPosy);

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(static_cast<float>(cursorPosx),
		static_cast<float>(cursorPosy));

	bool mousePressed[5];
	for (uint32_t i = 0; i < 5; ++i)
	{
        const auto pressed = glfwGetMouseButton(win, static_cast<int>(i));

		mousePressed[i] = pressed == GLFW_PRESS;
	}
	memcpy(&io.MouseDown[0], &mousePressed[0], sizeof(bool) * 5);

	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(win, &w, &h);
	glfwGetFramebufferSize(win, &display_w, &display_h);
	io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w) / w, static_cast<float>(display_h) / h);

	ImGuiOptions optionsCopy = graphicsOptions;

	ImGui::NewFrame();

	ImGui::Begin("options");

    ImGui::BeginGroup();

        ImGui::RadioButton("Deferred", &graphicsOptions.mToggleIndex, 0);
        ImGui::RadioButton("Forward", &graphicsOptions.mToggleIndex, 1);

    ImGui::EndGroup();

    if (graphicsOptions.mToggleIndex == 0)
    {
        graphicsOptions.mDefered = true;
        graphicsOptions.mForward = false;
    }
    else
    {
        graphicsOptions.mDefered = false;
        graphicsOptions.mForward = true;
    }

    ImGui::Checkbox("Show lights", &graphicsOptions.mShowLights);

    ImGui::Text("Camera position: X: %f Y: %f Z: %f", cam.getPosition().x, cam.getPosition().y, cam.getPosition().z);


	ImGui::End();

	ImGui::Render();

	return std::memcmp(&graphicsOptions, &optionsCopy, sizeof(ImGuiOptions)) != 0;
}


int main(int argc, char** argv)
{
    srand((uint32_t)time(nullptr));

    const uint32_t windowWidth = 1440;
    const uint32_t windowHeight = 810;

    bool firstFrame = true;

    // initialse glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.DisplaySize = ImVec2{ windowHeight, windowWidth };

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Pass registration example", nullptr, nullptr);
    if(window == nullptr)
    {
        glfwTerminate();
        return 5;
    }

    Engine engine{window};

    engine.startFrame();

    std::array<std::string, 6> skybox{"./Assets/Skybox.png", "./Assets/Skybox.png", "./Assets/Skybox.png", "./Assets/Skybox.png", "./Assets/Skybox.png", "./Assets/Skybox.png"};

    Scene testScene("./Assets/Sponza/sponzaPBR.obj");
    testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, &engine);
    testScene.loadSkybox(skybox, &engine);
    testScene.setShadowingLight(float3(10.0f, 10.0f, 10.0f), float3(0.0f, 0.0f, 1.0f));
    testScene.uploadData(&engine);
    testScene.computeBounds(MeshType::Static);
    testScene.computeBounds(MeshType::Dynamic);
    
    {
        const AABB sceneBounds = testScene.getBounds();

        for (float x = sceneBounds.getBottom().x; x < sceneBounds.getTop().x; x += 300.0f)
        {
            for (float y = sceneBounds.getBottom().y; y < sceneBounds.getTop().y; y += 300.0f)
            {
                for (float z = sceneBounds.getBottom().z; z < sceneBounds.getTop().z; z += 300.0f)
                {
                    const float r = float(rand()) / float((RAND_MAX));
                    const float g = float(rand()) / float((RAND_MAX));
                    const float b = float(rand()) / float((RAND_MAX));

                    testScene.addLight({float4(x, y, z, 1.0f), float4(), float4(r, g, b, 1.0f), 20000.0f, 300.0f, LightType::Point, 0.0f});
                }
            }
        }
    }

    engine.setScene(testScene);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        Camera& camera = engine.getCurrentSceneCamera();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.moveForward(1.5f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.moveBackward(1.5f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.moveLeft(1.5f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.moveRight(1.5f);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.rotateYaw(1.0f);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.rotateYaw(-1.0f);
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.moveUp((1.5f));
        if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            camera.moveDown(1.5f);

		bool unregisterpasses = false;

		if (!firstFrame)
		{
			engine.startFrame();
            unregisterpasses = renderMenu(window, camera);
		}

        auto& scene = engine.getScene();

		if(unregisterpasses)
			engine.clearRegisteredPasses();


        if (graphicsOptions.mShowLights)
            engine.registerPass(PassType::LightFroxelation);
		
        if (graphicsOptions.mDefered)
		{
            engine.registerPass(PassType::GBuffer);
            engine.registerPass(PassType::DeferredPBRIBL);

            if (graphicsOptions.mShowLights)
                engine.registerPass(PassType::DeferredAnalyticalLighting);

            engine.registerPass(PassType::SSAOImproved);
		}
		else if(graphicsOptions.mForward)
        {
            engine.registerPass(PassType::DepthPre);
        
            if (graphicsOptions.mShowLights)
                engine.registerPass(PassType::ForwardCombinedLighting);
            else
                engine.registerPass(PassType::ForwardIBL);

            engine.registerPass(PassType::SSAO);
        }

        engine.registerPass(PassType::ConvolveSkybox);
        engine.registerPass(PassType::Skybox);
        engine.registerPass(PassType::DFGGeneration);
        engine.registerPass(PassType::Shadow);
        engine.registerPass(PassType::TAA);
        engine.registerPass(PassType::Overlay);
		engine.registerPass(PassType::Composite);

        engine.recordScene();
        engine.render();
        engine.swap();
        engine.endFrame();

        firstFrame = false;
    }

}
