#ifndef MESH_PICKER_HPP
#define MESH_PICKER_HPP

#include <cstdint>

#include "Engine/Scene.h"

class CPURayTracingScene;
class Camera;


class MeshPicker
{
public:

    MeshPicker()  :
        mRayTracedScene(nullptr),
        mSelectedMeshInstance(kInvalidInstanceID) {}

    MeshPicker(CPURayTracingScene* rt) :
        mRayTracedScene(rt),
        mSelectedMeshInstance(kInvalidInstanceID) {}

    ~MeshPicker() = default;

    void setScene(CPURayTracingScene* scene)
    {
        if(scene == nullptr)
            mSelectedMeshInstance = kInvalidInstanceID;
        mRayTracedScene = scene;
    }

    void tick(const Camera& cam);

    int64_t getCurrentlySelectedMesh() const
    {
        return mSelectedMeshInstance;
    }

    void clearSelected()
    {
        mSelectedMeshInstance = kInvalidInstanceID;
    }

    void setInstance(const InstanceID id)
    {
        mSelectedMeshInstance = id;
    }

private:

    CPURayTracingScene* mRayTracedScene;
    int64_t mSelectedMeshInstance;
};

#endif
