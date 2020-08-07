#ifndef DEBUG_VISUALIZATION_TECHNIQUE_HPP
#define DEBUG_VISUALIZATION_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"


class DebugAABBTechnique : public Technique
{
public:
    DebugAABBTechnique(Engine*, RenderGraph&);
    ~DebugAABBTechnique() = default;

    virtual PassType getPassType() const
    {
        return PassType::DebugAABB;
    }

    virtual void render(RenderGraph&, Engine*) {}

    virtual void bindResources(RenderGraph&) {}

private:
    TaskID mTaskID;
    GraphicsPipelineDescription mPipelineDesc;
    Shader mDebugVisVertexShader;
    Shader mDebugVisFragmentShader;

    Buffer mVertexBuffer;
    BufferView mVertexBufferView;

    Buffer mIndexBuffer;
    BufferView mIndexBufferView;
};

#endif
