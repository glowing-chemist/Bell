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

    CommandContextBase(RenderDevice* dev, const QueueType);
    virtual ~CommandContextBase();

    virtual void setupState(const RenderGraph&, uint32_t taskIndex, Executor* exec, const uint64_t prefixHash) = 0;

    virtual Executor* allocateExecutor(const bool timeStamp = false) = 0;
    virtual void      freeExecutor(Executor*) = 0;

    virtual const std::vector<uint64_t>& getTimestamps() = 0;
    virtual void      reset() = 0;

    QueueType getQueueType() const
    {
        return mQueueType;
    }

protected:

    std::vector<Executor*> mFreeExecutors;
    QueueType mQueueType;

};

#endif
