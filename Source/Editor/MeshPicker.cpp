#include "MeshPicker.hpp"
#include "Engine/InstanceIDTechnique.hpp"
#include "imgui.h"

#include <glm/glm.hpp>

void MeshPicker::tick(const uint2& pos)
{
    if(mIDTechnique && !mFirstFrame)
    {
        mIDTechnique->setMousePosition(pos);
        mSelectedMeshInstance = mIDTechnique->getCurrentlySelectedInstanceID();
    }

    if(mIDTechnique && mFirstFrame)
        mFirstFrame = false;
}
