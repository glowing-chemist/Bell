#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

void GraphicsTask::recordCommands(Executor& exec, const RenderGraph& graph) const
{
	if(graph.getVertexBuffer())
		exec.bindVertexBuffer(*graph.getVertexBuffer(), mVertexBufferOffset);

    for(const auto& thunk : mDrawCalls)
    {
        switch (thunk.mDrawType)
        {
            case DrawType::Standard:
			{
				exec.draw(thunk.mVertexOffset, thunk.mNumberOfVerticies);
				break;
			}

            case DrawType::Indexed:
			{
				exec.indexedDraw(thunk.mVertexOffset, thunk.mIndexOffset, thunk.mNumberOfIndicies);
				break;
			}

            case DrawType::Instanced:
			{
				exec.instancedDraw(thunk.mVertexOffset, thunk.mNumberOfVerticies, thunk.mNumberOfInstances);
				break;
			}

            case DrawType::Indirect:
			{
				exec.indirectDraw(thunk.mNumberOfInstances, graph.getBoundBuffer(thunk.mIndirectBufferName));
				break;
			}

            case DrawType::IndexedInstanced:
			{
				exec.indexedInstancedDraw(thunk.mVertexOffset, thunk.mIndexOffset, thunk.mNumberOfInstances, thunk.mNumberOfIndicies);
				break;
			}

            case DrawType::IndexedIndirect:
			{
				exec.indexedIndirectDraw(thunk.mNumberOfInstances, graph.getBoundBuffer(thunk.mIndirectBufferName));
				break;
			}

			case DrawType::SetPushConstant:
				exec.insertPushConsatnt(thunk.mPushConstantValue);

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


std::vector<vk::ClearValue> GraphicsTask::getClearValues() const
{
    std::vector<vk::ClearValue> clearValues;

    for(const auto& attatchment : mOutputAttachments)
    {
            if(attatchment.mLoadOp == LoadOp::Clear_Black)
            {
                clearValues.emplace_back(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
            }
			else if(attatchment.mLoadOp == LoadOp::Clear_White)
            {
                clearValues.emplace_back(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f});
            }
			else // coprresponds to Preserve so this value will be ignored.
			{
				clearValues.emplace_back(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
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

