#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"
#include "imguifilebrowser.h"
#include "ImGuiNodeEditor.h"
#include "ImGuizmo.h"
#include "MeshPicker.hpp"

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
    float2 size;
};

enum EditorMode
{
	SceneView = 0,
	NodeEditor = 1
};

struct CursorPosition
{
    double x;
    double y;
};

struct StaticMeshEntry
{
    SceneID mID;
    std::string mName;
};

class Editor;

class EditorTab
{
public:
    EditorTab(const std::string& name);
    virtual ~EditorTab() = default;

    // Return true to recompile renderGraph.
    virtual bool renderTab(Scene* currentScene,
                           CPURayTracingScene* rayTracedScene,
                           RenderGraph& gaph,
                           Editor* editor) = 0;

    const std::string& getName() const
    {
        return mName;
    }

private:

    std::string mName;
};

class Editor
{
public:
    Editor(GLFWwindow*);
    ~Editor();

    void run();

    void mouseScroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void text_callback(GLFWwindow* window, unsigned int codePoint);

    void loadSceneOnStartup(const std::string& path);

    void addTab(EditorTab* tab)
    {
        mTabExtensions.push_back(tab);
    }

    EditorTab* getTab(const std::string& name)
    {
        auto tab = std::find_if(mTabExtensions.begin(), mTabExtensions.end(), [name](const auto* t)
        {
           return t->getName() == name;
        });

        BELL_ASSERT(tab != mTabExtensions.end(), "Unable to fin dtab by that name")

        return *tab;
    }

private:

    void startFrame(const std::chrono::microseconds deltaFrameTime)
    { mEngine.startFrame(deltaFrameTime); }

	void endFrame()
	{ mEngine.endFrame(); }

    void renderScene();
    void renderOverlay();
    void swap();

    void pumpInputQueue();

    void drawMenuBar();
    void drawAssistantWindow();
    void drawMeshSelctorWindow();
    void drawDebugTexturePicker(const std::vector<const char*>& textures);
    void drawLightMenu();
    bool drawGuizmo(float4x4& world, const float4x4& view, const float4x4& proj, const ImGuizmo::OPERATION mode);

    void addMaterial();
    void drawAddInstanceDialog();

    void drawPassContextMenu(const PassType);

    void bakeAndSaveLightProbes();

    void loadScene(const std::string&);

    void updateMeshInstancePicker();
    void drawselectedMeshGuizmo();

    void deleteSelectedMesh();

    GLFWwindow* mWindow;

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

    bool mImGuiSetupNeeded;

    RenderEngine mEngine;

    std::vector<InstanceID> mSceneInstanceIDs;

    Scene* mInProgressScene;
    CPURayTracingScene* mRayTracingScene;

    std::vector<StaticMeshEntry> mStaticMeshEntries;
    bool mShowMeshFileBrowser;

    bool mShowMaterialDialog;
    char mMeshInstanceScratchBuffer[32];
    ImGuiMaterialDialog mMaterialDialog;
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
    std::vector<RenderEngine::IrradianceProbeVolume> mIrradianceVolumes;
    std::vector<RenderEngine::RadianceProbe> mRadianceProbes;

    std::vector<EditorLight> mLights;
    EditorLight mShadowingLight;
    ImGuizmo::OPERATION mLightOperationMode;
    bool mEditShadowingLight;

    MeshPicker mMeshPicker;
    InstanceID mSelectedMesh;

    Camera mMainCamera;
    Camera mShadowCamera;

    std::string mInitialScene;

    std::vector<EditorTab*> mTabExtensions;
};

#endif
