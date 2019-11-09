#include <GLFW/glfw3.h>
#include <vector>
#include <string>

#include "Engine/Engine.hpp"


int main(int argc, char** argv)
{
    const uint32_t windowWidth = 800;
    const uint32_t windowHeight = 600;

    bool firstFrame = true;

    // initialse glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);


    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Pass registration example", nullptr, nullptr);
    if(window == nullptr)
    {
        glfwTerminate();
        return 5;
    }

    Engine engine{window};

    engine.startFrame();

    BELL_ASSERT(argc > 1, "Scene file input needed")

    std::array<std::string, 6> skybox{"./skybox/peaks_lf.tga", "./skybox/peaks_rt.tga", "./skybox/peaks_up.tga", "./skybox/peaks_dn.tga", "./skybox/peaks_ft.tga", "./skybox/peaks_bk.tga"};

    Scene testScene(argv[1]);
    testScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, &engine);
    testScene.loadSkybox(skybox, &engine);

    engine.setScene(testScene);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        Camera& camera = engine.getCurrentSceneCamera();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.moveForward(0.5f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.moveBackward(0.5f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.moveLeft(0.5f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.moveRight(0.5f);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.rotateYaw(1.0f);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.rotateYaw(-1.0f);
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.moveUp((0.5f));
        if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            camera.moveDown(0.5f);

        if(!firstFrame)
            engine.startFrame();


        engine.registerPass(PassType::ConvolveSkybox);
        engine.registerPass(PassType::GBufferMaterial);
        engine.registerPass(PassType::DeferredTextureAnalyticalPBRIBL);
        engine.registerPass(PassType::Skybox);

        engine.recordScene();
        engine.render();
        engine.swap();
        engine.endFrame();
        engine.flushWait();

        firstFrame = false;
    }

}
