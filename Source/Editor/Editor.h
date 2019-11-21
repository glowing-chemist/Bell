#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"
#include "imguifilebrowser.h"
#include "ImGuiNodeEditor.h"

#include "GLFW/glfw3.h"

enum EditorMode
{
	SceneView = 0,
	NodeEditor = 1
};

class Editor
{
public:
    Editor(GLFWwindow*);
    ~Editor();

    void run();

private:

	void startFrame()
	{ mEngine.startFrame(); }

	void endFrame()
	{ mEngine.endFrame(); }

    void renderScene();
    void renderOverlay();
    void swap();

    void pumpInputQueue();

    void drawMenuBar();
    void drawAssistantWindow();

    void drawPassContextMenu(const PassType);

	void loadScene(const std::string&);

    GLFWwindow* mWindow;

    struct CursorPosition
    {
        double x;
        double y;
    };
    CursorPosition mCurrentCursorPos;
    int mMode;
    bool mShowHelpMenu;

    bool mShowFileBrowser;
    ImGuiFileBrowser mFileBrowser;
    bool mShowNodeEditor;
    ImGuiNodeEditor mNodeEditor;

	bool mSetupNeeded;

    Engine mEngine;

    Scene mInProgressScene;
};

#endif
