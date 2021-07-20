#ifndef VULKAN_ACCELERATION_STRUCTURES_HPP
#define VULKAN_ACCELERATION_STRUCTURES_HPP

#include <optional>

#include "Core/AccelerationStructures.hpp"
#include "Core/Vulkan/VulkanBuffer.hpp"

class MeshInstance;

struct VulkanAccelerationStructure
{
    vk::AccelerationStructureKHR mAccelerationHandle;
    Buffer mBackingBuffer;
};

class VulkanBottomLevelAccelerationStructure : public BottomLevelAccelerationStructureBase
{
public:
    VulkanBottomLevelAccelerationStructure(RenderEngine*, const StaticMesh&, const std::string&);
    VulkanBottomLevelAccelerationStructure(Executor*, const StaticMesh&, const std::string& = "");
    ~VulkanBottomLevelAccelerationStructure();

    const vk::AccelerationStructureKHR& getAccelerationStructureHandle() const
    {
        return mBVH.mAccelerationHandle;
    }

    Buffer& getBackingBuffer()
    {
        return mBVH.mBackingBuffer;
    }

    const Buffer& getBackingBuffer() const
    {
        return mBVH.mBackingBuffer;
    }

private:

    VulkanAccelerationStructure constructAccelerationStructure(RenderEngine*, const StaticMesh&, const std::string& name);

    VulkanAccelerationStructure mBVH;

};


class VulkanTopLevelAccelerationStructure : public TopLevelAccelerationStructureBase
{
public:
    VulkanTopLevelAccelerationStructure(RenderEngine*);
    ~VulkanTopLevelAccelerationStructure();

    virtual void reset() override final;

    virtual void addInstance(const MeshInstance*) override final;

    virtual void buildStructureOnCPU(RenderEngine*) override final;
    virtual void buildStructureOnGPU(Executor*) override final;

    const vk::AccelerationStructureKHR& getAccelerationStructureHandle() const
    {
        return (*mBVH).mAccelerationHandle;
    }

    Buffer& getBackingBuffer()
    {
        return (*mBVH).mBackingBuffer;
    }

    const Buffer& getBackingBuffer() const
    {
        return (*mBVH).mBackingBuffer;
    }

private:

    std::vector<const MeshInstance*> mInstances;
    std::unordered_map<const MeshInstance*, vk::AccelerationStructureKHR> mBottomLevelStructures;

    std::optional<VulkanAccelerationStructure> mBVH;

};

#endif