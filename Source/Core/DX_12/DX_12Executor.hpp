#ifndef DX_12_EXECUTOR_HPP
#define DX_12_EXECUTOR_HPP

#include "Core/Executor.hpp"

#include <d3d12.h>


class DX12_Executor : public Executor
{
public:
    DX12_Executor(RenderDevice*, ID3D12GraphicsCommandList*);
    virtual ~DX12_Executor() {};

    virtual void draw(const uint32_t vertexOffset, const uint32_t vertexCount) override final;

    virtual void instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount) override final;

    virtual void indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) override final;

    virtual void indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies) override final;

    virtual void indirectDraw(const uint32_t drawCalls, const BufferView&) override final;

    virtual void indexedIndirectDraw(const uint32_t drawCalls, const BufferView&) override final;

    virtual void insertPushConstant(const void* val, const size_t size) override final;

    virtual void dispatch(const uint32_t x, const uint32_t y, const uint32_t z) override final;

    virtual void dispatchIndirect(const BufferView&) override final;

    virtual void bindVertexBuffer(const BufferView&, const size_t offset) override final;

    virtual void bindVertexBuffer(const BufferView*, const size_t* offsets, const uint32_t) override final;

    virtual void bindIndexBuffer(const BufferView&, const size_t offset) override final;

    virtual void recordBarriers(BarrierRecorder&) override final;

    virtual void startCommandPredication(const BufferView&, const uint32_t index) override final;

    virtual void endCommandPredication() override final;

    virtual void setGraphicsShaders(const GraphicsTask &task,
                                    const RenderGraph& graph,
                                    const Shader& vertexShader,
                                    const Shader* geometryShader,
                                    const Shader* tessControl,
                                    const Shader* tessEval,
                                    const Shader& fragmentShader) override final;

    virtual void setComputeShader(const ComputeTask& task,
                                  const RenderGraph& graph,
                                  const Shader&) override final;

    virtual void setGraphicsPipeline(const uint64_t) override final;
    virtual void setComputePipeline(const uint64_t) override final;

    // Commands for updating resources
    virtual void copyDataToBuffer(const void*, const size_t size, const size_t offset, Buffer&) override final;

    virtual void blitImage(const ImageView& dst, const ImageView& src, const SamplerType) override final;

private:
    ID3D12GraphicsCommandList* mCommandList;
};

#endif