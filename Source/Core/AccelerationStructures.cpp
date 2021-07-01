#include "AccelerationStructures.hpp"
#include "Engine/Engine.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanAccelerationStructures.hpp"
#elif defined(DX_12)
// TODO
#endif

BottomLevelAccelerationStructureBase::BottomLevelAccelerationStructureBase(RenderEngine* eng, const StaticMesh &,
                                                                           const std::string &) :
        DeviceChild(eng->getDevice()),
        GPUResource(getDevice()->getCurrentSubmissionIndex())
{}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderEngine* eng, const StaticMesh& mesh,
                                                                   const std::string& name)
{
#ifdef VULKAN
        mBase = std::make_shared<VulkanBottomLevelAccelerationStructure>(eng, mesh, name);
#elif defined(DX_12)
    // TODO
#endif
}

TopLevelAccelerationStructureBase::TopLevelAccelerationStructureBase(RenderEngine* engine) :
        DeviceChild(engine->getDevice()),
        GPUResource(getDevice()->getCurrentSubmissionIndex())
{}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(RenderEngine* eng)
{
#ifdef VULKAN
    mBase = std::make_shared<VulkanTopLevelAccelerationStructure>(eng);
#elif defined(DX_12)
    // TODO
#endif
}