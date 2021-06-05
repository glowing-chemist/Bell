#include "AccelerationStructures.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanAccelerationStructures.hpp"
#elif defined(DX_12)
// TODO
#endif

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(RenderEngine& eng, const StaticMesh& mesh,
                                                                   const std::string& name)
                                                                   {
#ifdef VULKAN
        mBase = std::make_shared<VulkanBottomLevelAccelerationStructure>(eng, mesh, name);
#elif defined(DX_12)
    // TODO
#endif
                                                                   }

TopLevelAccelerationStructure::TopLevelAccelerationStructure(RenderEngine& eng)
{
#ifdef VULKAN
    mBase = std::make_shared<VulkanTopLevelAccelerationStructure>(eng);
#elif defined(DX_12)
    // TODO
#endif
}