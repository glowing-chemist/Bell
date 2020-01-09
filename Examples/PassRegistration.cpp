#include <GLFW/glfw3.h>
#include <vector>
#include <string>

#include <imgui.h>

#include "Engine/Engine.hpp"

struct ImGuiOptions
{
	bool useLUT = true;
};

static ImGuiOptions graphicsOptions;

bool renderMenu(GLFWwindow* win)
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

	ImGui::SetNextWindowSize({ 200, 300 });

	ImGui::Begin("options");

	ImGui::Checkbox("Enable DFGLUT", &graphicsOptions.useLUT);

	ImGui::End();

	ImGui::Render();

	return std::memcmp(&graphicsOptions, &optionsCopy, sizeof(ImGuiOptions)) != 0;
}


int main(int argc, char** argv)
{
    const uint32_t windowWidth = 1200;
    const uint32_t windowHeight = 1200;

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

    BELL_ASSERT(argc > 1, "Scene file input needed")

    std::array<std::string, 6> skybox{"./skybox/peaks_ft.tga", "./skybox/peaks_bk.tga", "./skybox/peaks_up.tga", "./skybox/peaks_dn.tga", "./skybox/peaks_rt.tga", "./skybox/peaks_lf.tga"};

    Scene testScene(argv[1]);
    testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, &engine);
    testScene.loadSkybox(skybox, &engine);

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
			unregisterpasses = renderMenu(window);
		}

		if(unregisterpasses)
			engine.clearRegisteredPasses();

#if 0
		engine.registerPass(PassType::GBuffer);
		engine.registerPass(PassType::DFGGeneration);
		engine.registerPass(PassType::LightFroxelation);
		engine.registerPass(PassType::DeferredAnalyticalLighting);

#else
        engine.registerPass(PassType::ConvolveSkybox);
		
		if (graphicsOptions.useLUT)
		{
            engine.registerPass(PassType::GBuffer);
			engine.registerPass(PassType::DFGGeneration);
            engine.registerPass(PassType::DeferredPBRIBL);
		}
		else
        {
            engine.registerPass(PassType::GBufferMaterial);
			engine.registerPass(PassType::DeferredTextureAnalyticalPBRIBL);

        }
        engine.registerPass(PassType::Skybox);

#endif
		engine.registerPass(PassType::Overlay);
		engine.registerPass(PassType::Composite);

        engine.recordScene();
        engine.render();
        engine.swap();
        engine.endFrame();
        engine.flushWait();

        firstFrame = false;
    }

}
