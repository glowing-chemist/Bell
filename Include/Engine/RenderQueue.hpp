#ifndef RENDER_QUEUE_HPP
#define RENDER_QUEUE_HPP

#include "Scene.h"

#include <vector>

class Camera;
class RenderEngine;

enum RenderViewIndex : uint8_t
{
    kRenderView_Main = 0,
    kRenderView_Shadow,
    kRenderView_ShadowCascade1,
    kRenderView_ShadowCascade2,
    kRenderView_ShadowCascade3,

    kRenderView_Count
};

class RenderView
{
public:

    RenderView(const RenderEngine* eng) :
        mEng{eng},
        mInstances() {}
    ~RenderView() = default;

    void updateView(const Camera&);

    const std::vector<MeshInstance*>& getViewInstances() const
    {
        return mInstances;
    }

    const std::vector<const MeshInstance*>& getViewConstInstances() const
    {
        return mConstInstances;
    }

private:

    const RenderEngine* mEng;

    std::vector<MeshInstance*> mInstances;
    std::vector<const MeshInstance*> mConstInstances;
};

#endif