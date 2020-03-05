#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/RenderGraph.hpp"


void GraphicsTask::recordCommands(Executor& exec, const RenderGraph& graph, const uint32_t taskIndex) const
{
    if(getVertexAttributes() > 0)
        exec.bindVertexBuffer(graph.getVertexBuffer(taskIndex), mVertexBufferOffset);

    if (getVertexAttributes() > 0)
        exec.bindIndexBuffer(graph.getIndexBuffer(taskIndex), 0);

    for(const auto& thunk : mDrawCalls)
    {
        switch (thunk.mDrawType)
        {
            case DrawType::Standard:
			{
                exec.draw(thunk.mData.mDrawData.mVertexOffset, thunk.mData.mDrawData.mNumberOfVerticies);
				break;
			}

            case DrawType::Indexed:
			{
                exec.indexedDraw(thunk.mData.mDrawData.mVertexOffset, thunk.mData.mDrawData.mIndexOffset, thunk.mData.mDrawData.mNumberOfIndicies);
				break;
			}

            case DrawType::Instanced:
			{
                exec.instancedDraw(thunk.mData.mDrawData.mVertexOffset, thunk.mData.mDrawData.mNumberOfVerticies, thunk.mData.mDrawData.mNumberOfInstances);
				break;
			}

            case DrawType::Indirect:
			{
                exec.indirectDraw(thunk.mData.mDrawData.mNumberOfInstances, graph.getBoundBuffer(thunk.mData.mDrawData.mIndirectBufferName));
				break;
			}

            case DrawType::IndexedInstanced:
			{
                exec.indexedInstancedDraw(thunk.mData.mDrawData.mVertexOffset, thunk.mData.mDrawData.mIndexOffset, thunk.mData.mDrawData.mNumberOfInstances, thunk.mData.mDrawData.mNumberOfIndicies);
				break;
			}

            case DrawType::IndexedIndirect:
			{
                exec.indexedIndirectDraw(thunk.mData.mDrawData.mNumberOfInstances, graph.getBoundBuffer(thunk.mData.mDrawData.mIndirectBufferName));
				break;
			}

			case DrawType::SetPushConstant:
                exec.insertPushConsatnt(thunk.mData.mPushConstants, sizeof(glm::mat4) * 2 + sizeof(uint32_t));

			break;
        }
    }
}


void GraphicsTask::mergeWith(const RenderTask& task)
{
	const auto& graphicsTask = static_cast<const GraphicsTask&>(task);

	mDrawCalls.insert(mDrawCalls.end(), graphicsTask.mDrawCalls.begin(), graphicsTask.mDrawCalls.end());

	if(graphicsTask.mInputAttachments.size() > mInputAttachments.size())
		mInputAttachments = graphicsTask.mInputAttachments;

	if(graphicsTask.mOutputAttachments.size() > mOutputAttachments.size())
		mOutputAttachments = graphicsTask.mOutputAttachments;
}


std::vector<ClearValues> GraphicsTask::getClearValues() const
{
    std::vector<ClearValues> clearValues;

    for(const auto& attatchment : mOutputAttachments)
    {
            if(attatchment.mLoadOp == LoadOp::Clear_Black)
            {
                clearValues.emplace_back(attatchment.mType, 0.0f, 0.0f, 0.0f, 0.0f);
            }
			else if(attatchment.mLoadOp == LoadOp::Clear_White)
            {
                clearValues.emplace_back(attatchment.mType, 1.0f, 1.0f, 1.0f, 1.0f);
            }
			else // coprresponds to Preserve so this value will be ignored.
			{
                clearValues.emplace_back(attatchment.mType, 0.0f, 0.0f, 0.0f, 1.0f);
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
		hash += stringHasher(desc.mVertexShader->getFilePath());

        if(desc.mGeometryShader)
			hash += stringHasher((*desc.mGeometryShader)->getFilePath());

        if(desc.mHullShader)
			hash += stringHasher((*desc.mHullShader)->getFilePath());

        if(desc.mTesselationControlShader)
			hash += stringHasher((*desc.mTesselationControlShader)->getFilePath());

		hash += stringHasher(desc.mFragmentShader->getFilePath());

		if (desc.mDepthWrite)
			hash = ~hash;

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

