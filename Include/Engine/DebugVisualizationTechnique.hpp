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

    virtual void render(RenderGraph&, RenderEngine*);

    virtual void bindResources(RenderGraph&) {}

private:
    TaskID mTaskID;
    GraphicsPipelineDescription mAABBPipelineDesc;
    Shader mAABBDebugVisVertexShader;
    Shader mAABBDebugVisFragmentShader;

    GraphicsPipelineDescription mWireFramePipelineDesc;
    Shader mSimpleTransformShader;

    GraphicsPipelineDescription mDebugLightsPipeline;
    Shader mLightDebugFragmentShader;

    Buffer mVertexBuffer;
    BufferView mVertexBufferView;

    PerFrameResource<Buffer> mDebugLineVertexBuffer;
    PerFrameResource<BufferView> mDebugLineVertexBufferView;

    Buffer mIndexBuffer;
    BufferView mIndexBufferView;
};

#endif
