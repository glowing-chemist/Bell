#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Engine/Engine.hpp"
#include "Engine/Scene.h"
#include "imguifilebrowser.h"

#include "GLFW/glfw3.h"


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

	void addMenuBar();


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

    Engine mEngine;

	bool mHasUploadedFonts;
	Image mOverlayFontTexture;
    ImageView mOverlayTextureView;
	Image mUITexture;
	ImageView mUIImageView;
	Buffer mOverlayTranslationUBO;
	Sampler mFontsSampler;

    Scene mInProgressScene;
};

#endif
