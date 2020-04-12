#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"
#include "imguifilebrowser.h"
#include "ImGuiNodeEditor.h"
#include "ImGuizmo.h"

#include "GLFW/glfw3.h"


struct EditorLight
{
    size_t mId;
    LightType mType;
    float4 mPosition;
    float4 mDirection;
    float4 mUp;
    float mColour[3];
    size_t mInfluence;
};

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

    void mouseScroll_callback(GLFWwindow* window, double xoffset, double yoffset);

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
    void drawDebugTexturePicker(const std::vector<std::string>& textures);
    void drawLightMenu();
    void drawGuizmo(EditorLight&, const glm::mat4& view, const glm::mat4& proj, const ImGuizmo::OPERATION mode);

    void drawPassContextMenu(const PassType);

	void loadScene(const std::string&);

    GLFWwindow* mWindow;

    struct CursorPosition
    {
        double x;
        double y;
    };
    CursorPosition mCurrentCursorPos;
    CursorPosition mCursorPosDelta;
    std::atomic<double> mMouseScrollAmount;
    int mMode;
    bool mShowHelpMenu;

    bool mShowDebugTexturePicker;
    int mCurrentDebugTexture;
    std::string mDebugTextureName;

    bool mRecompileGraph;
    bool mInFreeFlyMode;

    bool mShowFileBrowser;
    ImGuiFileBrowser mFileBrowser;
    bool mShowNodeEditor;
    ImGuiNodeEditor mNodeEditor;
    uint64_t mRegisteredNodes;

	bool mSetupNeeded;

    Engine mEngine;

    Scene mInProgressScene;

    std::vector<EditorLight> mLights;
    EditorLight mShadowingLight;
    ImGuizmo::OPERATION mLightOperationMode;
};

#endif
