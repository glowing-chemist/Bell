#ifndef VULKAN_ACCELERATION_STRUCTURES_HPP
#define VULKAN_ACCELERATION_STRUCTURES_HPP

#include <optional>

#include "Core/AccelerationStructures.hpp"
#include "Core/Vulkan/VulkanBuffer.hpp"

struct VulkanAccelerationStructure
{
    vk::AccelerationStructureKHR mAccelerationHandle;
    VulkanBuffer mBackingBuffer;
};

class VulkanBottomLevelAccelerationStructure : public BottomLevelAccelerationStructureBase
{
public:
    VulkanBottomLevelAccelerationStructure(RenderEngine*, const StaticMesh&, const std::string&);
    ~VulkanBottomLevelAccelerationStructure();

    const vk::AccelerationStructureKHR& getAccelerationStructureHandle() const
    {
        return mBVH.mAccelerationHandle;
    }

    VulkanBuffer& getBackingBuffer()
    {
        return mBVH.mBackingBuffer;
    }

    const VulkanBuffer& getBackingBuffer() const
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

    virtual void addBottomLevelStructure(const BottomLevelAccelerationStructure&) override final;

    virtual void buildStructureOnCPU() override final;
    virtual void buildStructureOnGPU(Executor*) override final;

    const vk::AccelerationStructureKHR& getAccelerationStructureHandle() const
    {
        return (*mBVH).mAccelerationHandle;
    }

    VulkanBuffer& getBackingBuffer()
    {
        return (*mBVH).mBackingBuffer;
    }

    const VulkanBuffer& getBackingBuffer() const
    {
        return (*mBVH).mBackingBuffer;
    }

private:

    std::vector<VulkanBuffer> mBottomLevelStructures;

    std::optional<VulkanAccelerationStructure> mBVH;

};

#endif