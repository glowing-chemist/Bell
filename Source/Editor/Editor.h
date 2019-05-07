#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"

#include "GLFW/glfw3.h"


class Editor
{
public:
    Editor(GLFWwindow*);
    ~Editor();

    void run();

private:

    void renderScene();
    void renderOverlay();
    void swap();

    void pumpInputQueue();


    GLFWwindow* mWindow;

    struct CursorPosition
    {
        double x;
        double y;
    };
    CursorPosition mCurrentCursorPos;

    Engine mEngine;

    Scene mInProgressScene;
};

#endif
