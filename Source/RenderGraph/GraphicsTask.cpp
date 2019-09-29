#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

void GraphicsTask::recordCommands(vk::CommandBuffer CmdBuffer, const RenderGraph& graph, const vulkanResources& resources) const
{
	bool instancedPipelineBound = false;
	auto bindInstancedPipeline = [&instancedPipelineBound, &CmdBuffer, &resources]()
	{
		if (!instancedPipelineBound)
		{
			CmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, std::static_pointer_cast<GraphicsPipeline>(resources.mPipeline)->getInstancedVariant());
			instancedPipelineBound = true;
		}
	};

    for(const auto& thunk : mDrawCalls)
    {
        switch (thunk.mDrawType)
        {
            case DrawType::Standard:
			{
				CmdBuffer.draw(thunk.mNumberOfVerticies,
					1,
					thunk.mVertexOffset,
					0);
				break;
			}

            case DrawType::Indexed:
			{
				CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
					1,
					thunk.mIndexOffset,
					static_cast<int32_t>(thunk.mVertexOffset),
					0);
				break;
			}

            case DrawType::Instanced:
			{
				bindInstancedPipeline();

				CmdBuffer.draw(thunk.mNumberOfVerticies,
					thunk.mNumberOfInstances,
					thunk.mVertexOffset,
					0);
				break;
			}

            case DrawType::Indirect:
			{
				CmdBuffer.drawIndirect(graph.getBoundBuffer(thunk.mIndirectBufferName).getBuffer(),
					0,
					thunk.mNumberOfInstances,
					sizeof(vk::DrawIndirectCommand));
				break;
			}

            case DrawType::IndexedInstanced:
			{
				bindInstancedPipeline();

				CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
					thunk.mNumberOfInstances,
					thunk.mIndexOffset,
					static_cast<int32_t>(thunk.mVertexOffset),
					0);
				break;
			}

            case DrawType::IndexedIndirect:
			{
				CmdBuffer.drawIndexedIndirect(graph.getBoundBuffer(thunk.mIndirectBufferName).getBuffer(),
					0,
					thunk.mNumberOfInstances,
					sizeof(vk::DrawIndexedIndirectCommand));
				break;
			}

			case DrawType::SetPushConstant:
				CmdBuffer.pushConstants(resources.mPipeline->getLayoutHandle(),
										vk::ShaderStageFlagBits::eAll,
										0,
										sizeof(glm::mat4),
										&thunk.mPushConstantValue);

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
                clearValues.emplace_back(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
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

