#include "DX_12Executor.hpp"
#include "DX_12BufferView.hpp"

#include <d3d12.h>


DX12_Executor::DX12_Executor(RenderDevice* device, ID3D12GraphicsCommandList* commandList) :
    Executor(device),
    mCommandList(commandList)
{}


void DX12_Executor::draw(const uint32_t vertexOffset, const uint32_t vertexCount)
{
    ++mRecordedCommands;
    mCommandList->DrawInstanced(vertexCount, 1, vertexOffset, 0);
}


void DX12_Executor::instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount)
{
    ++mRecordedCommands;
    mCommandList->DrawInstanced(vertexCount, instanceCount, vertexOffset, 0);
}


void DX12_Executor::indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies)
{
    ++mRecordedCommands;
    mCommandList->DrawIndexedInstanced(numberOfIndicies, 1, indexOffset, vertexOffset, 0);
}


void DX12_Executor::indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
{
    ++mRecordedCommands;
    mCommandList->DrawIndexedInstanced(numberOfIndicies, 1, indexOffset, vertexOffset, 0);
}

void DX12_Executor::indirectDraw(const uint32_t drawCalls, const BufferView& buffer)
{

}


void DX12_Executor::indexedIndirectDraw(const uint32_t drawCalls, const BufferView& buffer)
{

}


void DX12_Executor::insertPushConsatnt(const void* val, const size_t size)
{

}


void DX12_Executor::dispatch(const uint32_t x, const uint32_t y, const uint32_t z)
{
    ++mRecordedCommands;
    mCommandList->Dispatch(x, y, z);
}


void DX12_Executor::dispatchIndirect(const BufferView&)
{

}


void DX12_Executor::bindVertexBuffer(const BufferView& view, const size_t offset)
{
    const DX_12BufferView* DX12View = static_cast<const DX_12BufferView*>(view.getBase());
    const D3D12_VERTEX_BUFFER_VIEW viewDesc = DX12View->getVertexBufferView(offset);
    mCommandList->IASetVertexBuffers(0, 1, &viewDesc);
}


void DX12_Executor::bindVertexBuffer(const BufferView* bufViews, const size_t* offsets, const uint32_t viewCount)
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> views(viewCount);
    for(uint32_t i = 0;i <viewCount; ++i)
    {
        const DX_12BufferView* DX12View = static_cast<const DX_12BufferView*>(bufViews[i].getBase());
        views[i] = DX12View->getVertexBufferView(offsets[i]);
    }

    mCommandList->IASetVertexBuffers(0, viewCount, views.data());
}


void DX12_Executor::bindIndexBuffer(const BufferView& view, const size_t offset)
{
    const DX_12BufferView* DX12View = static_cast<const DX_12BufferView*>(view.getBase());
    const D3D12_INDEX_BUFFER_VIEW viewDesc = DX12View->getIndexBufferView(offset);
    mCommandList->IASetIndexBuffer(&viewDesc);
}


void DX12_Executor::recordBarriers(BarrierRecorder&)
{

}


void DX12_Executor::startCommandPredication(const BufferView&, const uint32_t index)
{

}


void DX12_Executor::endCommandPredication()
{

}


void DX12_Executor::setGraphicsShaders(const GraphicsTask &task,
                                const RenderGraph& graph,
                                const Shader& vertexShader,
                                const Shader* geometryShader,
                                const Shader* tessControl,
                                const Shader* tessEval,
                                const Shader& fragmentShader)
{

}


void DX12_Executor::setComputeShader(const ComputeTask& task,
                              const RenderGraph& graph,
                              const Shader&)
{

}


void DX12_Executor::setGraphicsPipeline(const uint64_t)
{

}


void DX12_Executor::setComputePipeline(const uint64_t)
{

}


// Commands for updating resources
void DX12_Executor::copyDataToBuffer(const void*, const size_t size, const size_t offset, Buffer&)
{

}


void DX12_Executor::blitImage(const ImageView& dst, const ImageView& src, const SamplerType)
{

}