#include "RenderGraph/GraphicsTask.hpp"


void GraphicsTask::recordCommands(vk::CommandBuffer CmdBuffer) const
{
    for(const auto& thunk : mDrawCalls)
    {
        switch (thunk.mDrawType)
        {
            case DrawType::Standard:
                CmdBuffer.draw(thunk.mNumberOfVerticies,
                               1,
                               thunk.mVertexOffset,
                               0);
                break;

            case DrawType::Indexed:
                CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
                                      1,
                                      thunk.mIndexOffset,
                                      static_cast<int32_t>(thunk.mVertexOffset),
                                      0);
                break;

            case DrawType::Instanced:
                CmdBuffer.draw(thunk.mNumberOfVerticies,
                               thunk.mNumberOfInstances,
                               thunk.mVertexOffset,
                               0);
                break;

            case DrawType::Indirect:
                // TODO implement indirect draw calls
                break;

            case DrawType::IndexedInstanced:
                CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
                                      thunk.mNumberOfInstances,
                                      thunk.mIndexOffset,
                                      static_cast<int32_t>(thunk.mVertexOffset),
                                      0);
                break;

            case DrawType::IndexedInstancedIndirect:
                // TODO implement indirect draw calls
                break;
        }
    }
}


void GraphicsTask::mergeDraws(GraphicsTask & task)
{
    mDrawCalls.insert(mDrawCalls.end(), task.mDrawCalls.begin(), task.mDrawCalls.end());

    if(task.mInputAttachments.size() > mInputAttachments.size())
        mInputAttachments = task.mInputAttachments;

    if(task.mOutputAttachments.size() > mOutputAttachments.size())
        mOutputAttachments = task.mOutputAttachments;
}


namespace std
{

    size_t hash<GraphicsPipelineDescription>::operator()(const GraphicsPipelineDescription& desc) const noexcept
    {
        std::hash<std::string> stringHasher{};

        size_t hash = 0;
        hash ^= stringHasher(desc.mVertexShader.getFilePath());

        if(desc.mGeometryShader)
            hash ^= stringHasher((desc.mGeometryShader->getFilePath()));

        if(desc.mHullShader)
            hash ^= stringHasher((desc.mHullShader->getFilePath()));

        if(desc.mTesselationControlShader)
            hash ^= stringHasher((desc.mTesselationControlShader->getFilePath()));

        hash ^= stringHasher(desc.mFragmentShader.getFilePath());


        return hash;
    }

}


bool operator==(const GraphicsTask& lhs, const GraphicsTask& rhs)
{
    std::hash<GraphicsPipelineDescription> hasher{};
    return hasher(lhs.getPipelineDescription()) == hasher(rhs.getPipelineDescription()) && rhs.mDrawCalls.size() == lhs.mDrawCalls.size();
}


bool operator==(const GraphicsPipelineDescription& lhs, const GraphicsPipelineDescription& rhs)
{
    std::hash<GraphicsPipelineDescription> hasher{};
    return hasher(lhs) == hasher(rhs);
}

