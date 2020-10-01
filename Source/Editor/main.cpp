
#include "Editor/Editor.h"

int main(int argc, char* *argv)
{

    glfwInit();

#if defined(VULKAN)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif defined(OPENGL)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#endif

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    const uint32_t windowWidth = mode->width;
    const uint32_t windowHeight = mode->height;

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // only resize explicitly
    auto* window = glfwCreateWindow(windowWidth, windowHeight, "Bell Editor", nullptr, nullptr);

    Editor editor{window};

    // Open scene from command line.
    if(argc > 1)
    {
        editor.loadSceneOnStartup(argv[1]);
    }

    editor.run();

    glfwTerminate();
}
