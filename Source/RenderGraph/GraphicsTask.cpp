#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

void GraphicsTask::recordCommands(vk::CommandBuffer CmdBuffer, const RenderGraph& graph) const
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
                CmdBuffer.drawIndirect(graph.getBoundBuffer(thunk.mIndirectBufferName).getBuffer(),
                                       0,
                                       thunk.mNumberOfInstances,
                                       100); // TODO workout what the correct stride should be (maybe pass it down and let user decide)
                break;

            case DrawType::IndexedInstanced:
                CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
                                      thunk.mNumberOfInstances,
                                      thunk.mIndexOffset,
                                      static_cast<int32_t>(thunk.mVertexOffset),
                                      0);
                break;

            case DrawType::IndexedIndirect:
                CmdBuffer.drawIndexedIndirect(graph.getBoundBuffer(thunk.mIndirectBufferName).getBuffer(),
                                              0,
                                              thunk.mNumberOfInstances,
                                              100); // TODO workout what the correct stride should be (maybe pass it down and let user decide)
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


std::vector<vk::ClearValue> GraphicsTask::getClearValues() const
{
    std::vector<vk::ClearValue> clearValues;

    for(const auto& attatchment : mOutputAttachments)
    {
        if(attatchment.mLoadOp != LoadOp::Preserve)
        {
            if(attatchment.mLoadOp == LoadOp::Clear_Black)
            {
                clearValues.emplace_back(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            }
            else
            {
                clearValues.emplace_back(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f});
            }
        }
    }

    return clearValues;
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

