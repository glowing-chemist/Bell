#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "Core/DeviceChild.hpp"
#include "Core/Sampler.hpp"


class Buffer;
class BufferView;
class ImageView;
class BarrierRecorder;
class Shader;
class GraphicsTask;
class ComputeTask;
class RenderGraph;


class Executor : public DeviceChild
{
public:
    Executor(RenderDevice* dev) :
        DeviceChild(dev),
        mSubmitFlag(false ){}
	virtual ~Executor() = default;

	virtual void draw(const uint32_t vertexOffset, const uint32_t vertexCount) = 0;

	virtual void instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount) = 0;

	virtual void indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) = 0;

	virtual void indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies) = 0;

    virtual void indirectDraw(const uint32_t drawCalls, const BufferView&) = 0;

	virtual void indexedIndirectDraw(const uint32_t drawCalls, const BufferView&) = 0;

    virtual void insertPushConstant(const void* val, const size_t size) = 0;

	virtual void dispatch(const uint32_t x, const uint32_t y, const uint32_t z) = 0;

	virtual void dispatchIndirect(const BufferView&) = 0;

    virtual void bindVertexBuffer(const BufferView&, const size_t offset) = 0;

    virtual void bindVertexBuffer(const BufferView*, const size_t* offsets, const uint32_t) = 0;

    virtual void bindIndexBuffer(const BufferView&, const size_t offset) = 0;

    virtual void recordBarriers(BarrierRecorder&) = 0;

    virtual void startCommandPredication(const BufferView&, const uint32_t index) = 0;

    virtual void endCommandPredication() = 0;

    virtual void setGraphicsShaders(const GraphicsTask &task,
                                    const RenderGraph& graph,
                                    const Shader& vertexShader,
                                    const Shader* geometryShader,
                                    const Shader* tessControl,
                                    const Shader* tessEval,
                                    const Shader& fragmentShader) = 0;

    virtual void setComputeShader(const ComputeTask& task,
                                  const RenderGraph& graph,
                                  const Shader&) = 0;

    virtual void setGraphicsPipeline(const uint64_t) = 0;
    virtual void setComputePipeline(const uint64_t) = 0;

    // Commands for updating resources
    virtual void copyDataToBuffer(const void*, const size_t size, const size_t offset, Buffer&) = 0;

    virtual void blitImage(const ImageView& dst, const ImageView& src, const SamplerType) = 0;

    uint32_t getRecordedCommandCount() const
    {
        return mRecordedCommands;
    }

    void resetRecordedCommandCount()
    {
        mRecordedCommands = 0;
    }

    void setSubmitFlag()
    {
        mSubmitFlag = true;
    }

    void clearSubmitFlag()
    {
        mSubmitFlag = false;
    }

    bool getSubmitFlag() const
    {
        return mSubmitFlag;
    }

protected:
    uint32_t mRecordedCommands;

private:
    bool mSubmitFlag;
};

#endif
