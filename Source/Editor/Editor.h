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
    float mRadius;
    float mIntensity;
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
    void text_callback(GLFWwindow* window, unsigned int codePoint);

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
    void drawDebugTexturePicker(const std::vector<const char*>& textures);
    void drawLightMenu();
    void drawGuizmo(EditorLight&, const float4x4& view, const float4x4& proj, const ImGuizmo::OPERATION mode);
    void drawGuizmo(float4x4& world, const float4x4& view, const float4x4& proj, const ImGuizmo::OPERATION mode);

    void addMaterial();
    void drawAddInstanceDialog();

    void drawPassContextMenu(const PassType);

    void bakeAndSaveLightProbes();

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

    struct CameraInfo
    {
        bool mInFreeFlyMode;
        float mCameraSpeed;
        float mNearPlaneDistance;
        float mFarPlaneDistance;
        float mFOV;
    };
    CameraInfo mCameraInfo;

    bool mShowFileBrowser;
    ImGuiFileBrowser mFileBrowser;
    bool mShowNodeEditor;
    ImGuiNodeEditor mNodeEditor;
    uint64_t mRegisteredNodes;

	bool mSetupNeeded;

    Engine mEngine;

    std::vector<InstanceID> mSceneInstanceIDs;

    Scene* mInProgressScene;
    RayTracingScene* mRayTracingScene;
    struct StaticMeshEntry
    {
        SceneID mID;
        std::string mName;
    };
    std::vector<StaticMeshEntry> mStaticMeshEntries;
    bool mShowMeshFileBrowser;

    bool mShowMaterialDialog;
    char mMeshInstanceScratchBuffer[32];
    ImGuiMaterialDialog mMaterialDialog;
    std::vector<std::string> mMaterialNames;
    uint32_t mCurrentMaterialIndex;
    SceneID mMeshToInstance;
    bool mShowAddInstanceDialog;
    bool mResetSceneAtEndOfFrame;
    bool mPublishedScene;

    float mAnimationSpeed;

    struct EditorIrradianceVolumeOptions
    {
        bool mShowImGuizmo;
    };
    std::vector<EditorIrradianceVolumeOptions> mIrradianceVolumesOptions;
    std::vector<Engine::IrradianceProbeVolume> mIrradianceVolumes;
    int3 mLightProbeLookupSize;

    std::vector<EditorLight> mLights;
    EditorLight mShadowingLight;
    ImGuizmo::OPERATION mLightOperationMode;
    bool mEditShadowingLight;
};

#endif
