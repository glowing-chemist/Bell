#ifndef ACCELERATION_STRUCTURES_HPP
#define ACCELERATION_STRUCTURES_HPP

#include <string>

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

class StaticMesh;
class RenderEngine;
class Executor;

class BottomLevelAccelerationStructureBase : public DeviceChild
{
public:

    BottomLevelAccelerationStructureBase(RenderEngine&, const StaticMesh&, const std::string& = "");
    ~BottomLevelAccelerationStructureBase() = default;
};


class BottomLevelAccelerationStructure
{
    BottomLevelAccelerationStructure(RenderEngine&, const StaticMesh&, const std::string& = "");
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


class TopLevelAccelerationStructureBase : public DeviceChild
{
public:

    TopLevelAccelerationStructureBase(RenderEngine&);
    ~TopLevelAccelerationStructureBase();

    virtual void addBottomLevelStructure(const BottomLevelAccelerationStructure&) = 0;

    virtual void buildStructureOnCPU() = 0;
    virtual void buildStructureOnGPU(Executor*) = 0;
};

class TopLevelAccelerationStructure
{
    TopLevelAccelerationStructure(RenderEngine&);
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