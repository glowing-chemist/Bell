#ifndef MESH_PICKER_HPP
#define MESH_PICKER_HPP

#include <cstdint>

#include "Engine/Scene.h"

class InstanceIDTechnique;


class MeshPicker
{
public:

    MeshPicker()  :
            mFirstFrame(true),
            mIDTechnique(nullptr),
        mSelectedMeshInstance(kInvalidInstanceID) {}

    MeshPicker(InstanceIDTechnique* tech) :
            mFirstFrame(true),
            mIDTechnique(tech),
        mSelectedMeshInstance(kInvalidInstanceID) {}

    void setIDTechnique(InstanceIDTechnique* tech)
    {
        mIDTechnique = tech;
    }

    ~MeshPicker() = default;

    void tick(const uint2& pos);

    InstanceID getCurrentlySelectedMesh() const
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

    bool mFirstFrame;
    InstanceIDTechnique* mIDTechnique;
    InstanceID mSelectedMeshInstance;
};

#endif
