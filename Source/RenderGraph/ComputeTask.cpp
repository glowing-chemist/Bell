#include "RenderGraph/ComputeTask.hpp"


void ComputeTask::recordCommands(vk::CommandBuffer CmdBuffer)
{
    for(const auto& thunk : mComputeCalls)
    {
        switch(thunk.mDispatchType)
        {
            case DispatchType::Standard:
                CmdBuffer.dispatch(thunk.x,
                                   thunk.y,
                                   thunk.z);
                break;

            case DispatchType::Indirect:
                // TODO
                break;
        }
    }
}
