#ifndef MESH_PICKER_HPP
#define MESH_PICKER_HPP

#include <cstdint>

class RayTracingScene;
class Camera;


class MeshPicker
{
public:

    MeshPicker(RayTracingScene* rt) :
        mRayTracedScene(rt),
        mSelectedMeshInstance(~0LL) {}

    ~MeshPicker() = default;

    void setScene(RayTracingScene* scene)
    {
        mRayTracedScene = scene;
    }

    void tick(const Camera& cam);

    int64_t getCurrentlySelectedMesh() const
    {
        return mSelectedMeshInstance;
    }

private:

    RayTracingScene* mRayTracedScene;
    int64_t mSelectedMeshInstance;
};

#endif
