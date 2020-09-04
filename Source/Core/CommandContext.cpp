#include "CommandContext.hpp"
#include "Executor.hpp"
#include "RenderDevice.hpp"

CommandContextBase::CommandContextBase(RenderDevice* dev, const QueueType type) :
    DeviceChild(dev),
    mQueueType(type)
{
}


CommandContextBase::~CommandContextBase()
{
    for(auto* exec : mFreeExecutors)
        delete exec;
}
