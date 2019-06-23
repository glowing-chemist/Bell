#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

void ComputeTask::recordCommands(vk::CommandBuffer CmdBuffer, const RenderGraph& graph, const vulkanResources &) const
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
                CmdBuffer.dispatchIndirect(graph.getBoundBuffer(thunk.mIndirectBuffer).getBuffer(),
                                           100);
                break;
        }
    }
}


void ComputeTask::mergeWith(const RenderTask& task)
{
	const auto& compueTask = static_cast<const ComputeTask&>(task);

	mComputeCalls.insert(mComputeCalls.end(), compueTask.mComputeCalls.begin(), compueTask.mComputeCalls.end());

	if(compueTask.mInputAttachments.size() > mInputAttachments.size())
		mInputAttachments = compueTask.mInputAttachments;

	if(compueTask.mOutputAttachments.size() > mOutputAttachments.size())
		mOutputAttachments = compueTask.mOutputAttachments;
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
    return hasher(lhs.getPipelineDescription()) == hasher(rhs.getPipelineDescription()) && lhs.mComputeCalls.size() == rhs.mComputeCalls.size();
}


bool operator==(const ComputePipelineDescription& lhs, const ComputePipelineDescription& rhs)
{
    std::hash<ComputePipelineDescription> hasher{};
    return hasher(lhs) == hasher(rhs);
}
