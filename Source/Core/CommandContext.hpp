#ifndef COMMAND_CONTEXT_HPP
#define COMMAND_CONTEXT_HPP

#include <memory>
#include <vector>

#include "DeviceChild.hpp"
#include "RenderGraph/RenderGraph.hpp"

class Executor;


class CommandContextBase : public DeviceChild
{
public:

    CommandContextBase(RenderDevice* dev);
    virtual ~CommandContextBase();

    virtual void setupState(const RenderGraph&, uint32_t taskIndex, Executor* exec, const uint64_t prefixHash) = 0;

    virtual Executor* allocateExecutor() = 0;
    virtual void      freeExecutor(Executor*) = 0;


    virtual void      reset() = 0;

protected:

    std::vector<Executor*> mFreeExecutors;

};

#endif
