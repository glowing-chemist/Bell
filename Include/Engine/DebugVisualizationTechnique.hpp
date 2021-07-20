#ifndef DEBUG_VISUALIZATION_TECHNIQUE_HPP
#define DEBUG_VISUALIZATION_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/PerFrameResource.hpp"


class DebugAABBTechnique : public Technique
{
public:
    DebugAABBTechnique(RenderEngine*, RenderGraph&);
    ~DebugAABBTechnique() = default;

    virtual PassType getPassType() const
    {
        return PassType::DebugAABB;
    }

    virtual void render(RenderGraph&, RenderEngine*) override final;

    virtual void bindResources(RenderGraph&) override final {}

    virtual void postGraphCompilation(RenderGraph&, RenderEngine*) override final;

private:
    TaskID mTaskID;
    TaskID mWireFrameTaskID;
    GraphicsPipelineDescription mAABBPipelineDesc;
    Shader mAABBDebugVisVertexShader;
    Shader mAABBDebugVisFragmentShader;

    GraphicsPipelineDescription mWireFramePipelineDesc;

    GraphicsPipelineDescription mDebugLightsPipeline;
    Shader mLightDebugFragmentShader;

    Buffer mVertexBuffer;
    BufferView mVertexBufferView;

    PerFrameResource<Buffer> mDebugLineVertexBuffer;
    PerFrameResource<BufferView> mDebugLineVertexBufferView;

    Buffer mIndexBuffer;
    BufferView mIndexBufferView;

    PipelineHandle mWireFramePipelines[2];
};

#endif
