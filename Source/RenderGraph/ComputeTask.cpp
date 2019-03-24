#include "RenderGraph/ComputeTask.hpp"


void ComputeTask::recordCommands(vk::CommandBuffer CmdBuffer) const
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


void ComputeTask::mergeDispatches(ComputeTask& task)
{
    mComputeCalls.insert(mComputeCalls.end(), task.mComputeCalls.begin(), task.mComputeCalls.end());
}


namespace std
{

    size_t hash<ComputePipelineDescription>::operator()(const ComputePipelineDescription& desc) const noexcept
    {
        std::hash<std::string> stringHasher{};

        size_t hash = 0;
        hash ^= stringHasher(desc.mComputeShader.getFilePath());


        return hash;
    }

}


bool operator==(const ComputeTask& lhs, const ComputeTask& rhs)
{
    std::hash<ComputePipelineDescription> hasher{};
    return hasher(lhs.getPipelineDescription()) == hasher(rhs.getPipelineDescription());
}
