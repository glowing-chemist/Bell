#include <GLFW/glfw3.h>
#include <vector>
#include <string>

#include <imgui.h>
#include "Engine/GeomUtils.h"
#include "Core/Profiling.hpp"
#include <glm/gtx/transform.hpp>

#include "Engine/Engine.hpp"

#define USE_RAY_TRACING 0

struct ImGuiOptions
{
    int mToggleIndex = 0;
    bool mDefered = true;
    bool mForward = false;
    bool mShowLights = false;
    bool mTAA = false;
    bool mSSAO = false;
    bool mShadows = true;
    int mShadowToggle = 0;
    bool mShadowMaps = true;
#if USE_RAY_TRACING
    bool mRayTracedShadows = false;
#endif
    int mReflectionsToggle = 0;
    bool mSSR = false;
#if USE_RAY_TRACING
    bool mRayTracedReflections = false;
#endif
    bool preDepth = true;
    bool occlusionCulling = false;
};

static ImGuiOptions graphicsOptions;

bool renderMenu(GLFWwindow* win, Camera& cam, RenderEngine* eng, const float cpuTime)
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
    ImGui::Checkbox("Enable TAA", &graphicsOptions.mTAA);
    ImGui::Checkbox("Enable SSAO", &graphicsOptions.mSSAO);
    ImGui::Checkbox("Enable SSR", &graphicsOptions.mSSR);
    ImGui::Checkbox("Enable shadows", &graphicsOptions.mShadows);
    if(graphicsOptions.mShadows)
    {
        ImGui::BeginGroup();

            ImGui::RadioButton("Shadow maps", &graphicsOptions.mShadowToggle, 0);
#if USE_RAY_TRACING
            ImGui::RadioButton("Ray traced shadows", &graphicsOptions.mShadowToggle, 1);
#endif
        ImGui::EndGroup();

        if (graphicsOptions.mShadowToggle == 0)
        {
            graphicsOptions.mShadowMaps = true;
#if USE_RAY_TRACING
            graphicsOptions.mRayTracedShadows = false;
#endif
        }
        else
        {
            graphicsOptions.mShadowMaps = false;
#if USE_RAY_TRACING
            graphicsOptions.mRayTracedShadows = true;
#endif
        }
    }

    ImGui::Checkbox("Enable pre depth", &graphicsOptions.preDepth);
    ImGui::Checkbox("Occlusion culling (experimental)", &graphicsOptions.occlusionCulling);

    ImGui::Text("Camera position: X: %f Y: %f Z: %f", cam.getPosition().x, cam.getPosition().y, cam.getPosition().z);
    ImGui::Text("Camera direction: X: %f Y: %f Z: %f", cam.getDirection().x, cam.getDirection().y, cam.getDirection().z);
    ImGui::Text("Camera up: X: %f Y: %f Z: %f", cam.getUp().x, cam.getUp().y, cam.getUp().z);
    float farPlane = cam.getFarPlane();
    ImGui::SliderFloat("Far plane", &farPlane, 10.0f, 2000.0f);
    cam.setFarPlane(farPlane);

    RenderDevice* dev = eng->getDevice();
    const RenderGraph& graph = eng->getRenderGraph();
    const std::vector<uint64_t> timeStamps = dev->getAvailableTimestamps();
    // There is a 3 frame delay on timestamps so graph could have been rebuilt by now.
    if(timeStamps.size() == graph.taskCount())
    {
        float totalGPUTime = 0.0f;
        for(uint32_t i = 0; i < timeStamps.size(); ++i)
        {
            const RenderTask& task = graph.getTask(i);
            const float taskTime = dev->getTimeStampPeriod() * (float(timeStamps[i]) / 1000000.0f);
            ImGui::Text("%s : %f ms", task.getName().c_str(), taskTime);
            totalGPUTime += taskTime;
        }

        ImGui::Text("Total GPU time %f ms", totalGPUTime);
        ImGui::Text("Total CPU time %f ms", cpuTime);
    }

	ImGui::End();

	ImGui::Render();

	return std::memcmp(&graphicsOptions, &optionsCopy, sizeof(ImGuiOptions)) != 0;
}


int main()
{
    srand((uint32_t)time(nullptr));

    bool firstFrame = true;

    // initialse glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    const uint32_t windowWidth = mode->width;
    const uint32_t windowHeight = mode->height;

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.DisplaySize = ImVec2{ static_cast<float>(windowHeight), static_cast<float>(windowWidth) };

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Pass registration example", nullptr, nullptr);
    if(window == nullptr)
    {
        glfwTerminate();
        return 5;
    }

    GraphicsOptions options
    {
        DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Subgroup | DeviceFeaturesFlags::Discrete
#if USE_RAY_TRACING
        | DeviceFeaturesFlags::RayTracing
#endif
        ,
        true
    };
    RenderEngine* engine = new RenderEngine{window, options};
    engine->setShadowMapResolution({1920.0f, 1080.0f});

    engine->startFrame(std::chrono::microseconds(0));

    std::array<std::string, 6> skybox{"./Assets/skybox/px.png",
                                      "./Assets/skybox/nx.png",
                                      "./Assets/skybox/py.png",
                                      "./Assets/skybox/ny.png",
                                      "./Assets/skybox/pz.png",
                                      "./Assets/skybox/nz.png"};

    Scene testScene("./Assets/Sponza/sponzaPBR.obj");
    testScene.setRootTransform(glm::scale(float3(0.01f, 0.01f, 0.01f)));
    testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo, engine);
    testScene.loadSkybox(skybox, engine);
    testScene.computeBounds(AccelerationStructure::StaticMesh);
    testScene.computeBounds(AccelerationStructure::DynamicMesh);

    // set camera aspect ratio.
    Camera camera{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, static_cast<float>(windowHeight) / static_cast<float>(windowWidth),
                  0.1f, 200.0f};
    testScene.setCamera(&camera);
    camera.setAspect(float(windowWidth) / float(windowHeight));
    camera.setFarPlane(40.0f);

    const float3 lightDirection = glm::normalize(float3(0.0f, -1.0f, 0.0f));
    const float3 ligthUp = float3(0.0f, 0.0f, 1.0f);
    Camera ShadowCamera(float3(0.0f, 16.0f, 0.0f), lightDirection, float(windowWidth) / float(windowHeight), 1.0f, 20.0f);
    ShadowCamera.setUp(ligthUp);
    ShadowCamera.setMode(CameraMode::Orthographic);
    ShadowCamera.setOrthographicSize({20.0f, 10.0f});
    testScene.setShadowingLight(&ShadowCamera);
    
    {
        const AABB sceneBounds = testScene.getBounds();
        const float3 sceneSize = sceneBounds.getSideLengths();

        for (float x = sceneBounds.getMin().x; x < sceneBounds.getMax().x; x += sceneSize.x / 10.0f)
        {
            for (float y = sceneBounds.getMin().y; y < sceneBounds.getMax().y; y += sceneSize.x / 10.0f)
            {
                for (float z = sceneBounds.getMin().z; z < sceneBounds.getMax().z; z += sceneSize.x / 10.0f)
                {
                    const float r = float(rand()) / float((RAND_MAX));
                    const float g = float(rand()) / float((RAND_MAX));
                    const float b = float(rand()) / float((RAND_MAX));

                    testScene.addLight(Scene::Light::pointLight(float4(x, y, z, 1.0f), float4(r, g, b, 1.0f), 40.0f, 2.0f));
                }
            }
        }
    }
    testScene.computeBounds(AccelerationStructure::Lights);

    engine->setScene(&testScene);

    auto lastCPUTime = std::chrono::system_clock::now();

    while(!glfwWindowShouldClose(window))
    {
        PROFILER_START_FRAME("Start frame");

        const auto currentTime = std::chrono::system_clock::now();
        const std::chrono::duration<float, std::ratio<1, 1000>> diff = currentTime - lastCPUTime;
        lastCPUTime = currentTime;

        glfwPollEvents();

        Camera& camera = engine->getCurrentSceneCamera();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.moveForward(0.1f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.moveBackward(0.1f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.moveLeft(0.1f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.moveRight(0.1f);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.rotateYaw(1.0f);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.rotateYaw(-1.0f);
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            camera.rotatePitch(1.0f);
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            camera.rotatePitch(-1.0f);
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.moveUp((0.1f));
        if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            camera.moveDown(0.1f);

		bool unregisterpasses = false;

		if (!firstFrame)
		{
            engine->startFrame(std::chrono::duration_cast<std::chrono::microseconds>(diff));
            unregisterpasses = renderMenu(window, camera, engine, diff.count());
		}

		if(unregisterpasses)
            engine->clearRegisteredPasses();


        engine->registerPass(PassType::ConvolveSkybox);
        engine->registerPass(PassType::Skybox);
        engine->registerPass(PassType::LineariseDepth);
        engine->registerPass(PassType::DebugAABB);
        //engine->registerPass(PassType::Voxelize);

        if (graphicsOptions.mForward || graphicsOptions.preDepth)
            engine->registerPass(PassType::DepthPre);

        if(graphicsOptions.mShadows && graphicsOptions.mShadowMaps)
            engine->registerPass(PassType::Shadow);

#if USE_RAY_TRACING
        if(engine->getDevice()->getDeviceFeatureFlags() & DeviceFeaturesFlags::RayTracing)
        {
            if (graphicsOptions.mShadows && graphicsOptions.mRayTracedShadows)
                engine->registerPass(PassType::RayTracedShadows);

            engine->registerPass(PassType::BuildAccelerationStructures);
        }
#endif
        if (graphicsOptions.mShowLights)
            engine->registerPass(PassType::LightFroxelation);
		
        if(graphicsOptions.mSSAO && graphicsOptions.mDefered)
            engine->registerPass(PassType::SSAO);

        if (graphicsOptions.mDefered)
		{
            if(graphicsOptions.preDepth)
                engine->registerPass(PassType::GBufferPreDepth);
            else
                engine->registerPass(PassType::GBuffer);

            engine->registerPass(PassType::DeferredPBRIBL);

            if (graphicsOptions.mShowLights)
                engine->registerPass(PassType::DeferredAnalyticalLighting);
		}
		else if(graphicsOptions.mForward)
        {        
            if (graphicsOptions.mShowLights)
                engine->registerPass(PassType::ForwardCombinedLighting);
            else
                engine->registerPass(PassType::ForwardIBL);
        }

        if (graphicsOptions.mTAA)
            engine->registerPass(PassType::TAA);

        if(graphicsOptions.occlusionCulling)
            engine->registerPass(PassType::OcclusionCulling);

        if (graphicsOptions.mSSR)
        {
            engine->registerPass(PassType::SSR);
            engine->registerPass(PassType::DownSampleColour);
        }

        engine->registerPass(PassType::DFGGeneration);
        engine->registerPass(PassType::Overlay);
        engine->registerPass(PassType::Composite);

        engine->recordScene();
        engine->render();
        engine->swap();
        engine->endFrame();

        firstFrame = false;
    }

}
