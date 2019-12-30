
#include "Editor/Editor.h"

int main()
{

    glfwInit();

#if defined(VULKAN)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif defined(OPENGL)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // only resize explicitly
    auto* window = glfwCreateWindow(1600, 900, "Bell Editor", nullptr, nullptr);

    Editor editor{window};

    editor.run();

    glfwTerminate();
}
