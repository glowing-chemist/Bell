
#include "Editor/Editor.h"

#include "imgui.h"

Editor::Editor(GLFWwindow* window) :
    mWindow{window},
    mCurrentCursorPos{0.0, 0.0},
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
    while(!glfwWindowShouldClose(mWindow))
    {
        pumpInputQueue();
    }
}

void Editor::renderScene()
{

}


void Editor::renderOverlay()
{

}


void Editor::swap()
{

}


void Editor::pumpInputQueue()
{
    glfwPollEvents();

    glfwGetCursorPos(mWindow, &mCurrentCursorPos.x, &mCurrentCursorPos.y);
}
