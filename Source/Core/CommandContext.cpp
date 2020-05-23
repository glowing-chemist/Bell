#include "CommandContext.hpp"
#include "Executor.hpp"
#include "RenderDevice.hpp"

CommandContextBase::CommandContextBase(RenderDevice* dev) :
    DeviceChild(dev)
{
}


CommandContextBase::~CommandContextBase()
{
    for(auto* exec : mFreeExecutors)
        delete exec;
}
