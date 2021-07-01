#ifndef ACCELERATION_STRUCTURES_HPP
#define ACCELERATION_STRUCTURES_HPP

#include <string>

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

class StaticMesh;
class MeshInstance;
class RenderEngine;
class Executor;

class BottomLevelAccelerationStructureBase : public DeviceChild, public GPUResource
{
public:

    BottomLevelAccelerationStructureBase(RenderEngine*, const StaticMesh&, const std::string& = "");
    ~BottomLevelAccelerationStructureBase() = default;
};


class BottomLevelAccelerationStructure
{
public:

    BottomLevelAccelerationStructure(RenderEngine*, const StaticMesh&, const std::string& = "");
    ~BottomLevelAccelerationStructure() = default;

    BottomLevelAccelerationStructureBase* getBase()
    {
        return mBase.get();
    }

    const BottomLevelAccelerationStructureBase* getBase() const
    {
        return mBase.get();
    }

    BottomLevelAccelerationStructureBase* operator->()
    {
        return getBase();
    }

    const BottomLevelAccelerationStructureBase* operator->() const
    {
        return getBase();
    }

private:

    std::shared_ptr<BottomLevelAccelerationStructureBase> mBase;
};


class TopLevelAccelerationStructureBase : public DeviceChild, public GPUResource
{
public:

    TopLevelAccelerationStructureBase(RenderEngine*);
    virtual ~TopLevelAccelerationStructureBase() = default;

    virtual void reset() = 0;

    virtual void addInstance(const MeshInstance*) = 0;

    virtual void buildStructureOnCPU(RenderEngine*) = 0;
    virtual void buildStructureOnGPU(Executor*) = 0;
};

class TopLevelAccelerationStructure
{
public:

    TopLevelAccelerationStructure(RenderEngine*);
    ~TopLevelAccelerationStructure() = default;

    TopLevelAccelerationStructureBase* getBase()
    {
        return mBase.get();
    }

    const TopLevelAccelerationStructureBase* getBase() const
    {
        return mBase.get();
    }

    TopLevelAccelerationStructureBase* operator->()
    {
        return getBase();
    }

    const TopLevelAccelerationStructureBase* operator->() const
    {
        return getBase();
    }

private:

    std::shared_ptr<TopLevelAccelerationStructureBase> mBase;
};

#endif