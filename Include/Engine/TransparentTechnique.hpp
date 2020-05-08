#ifndef TRANSPARENT_TECHNIQUE_HPP
#define TRANSPARENT_TECHNIQUE_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class TransparentTechnique : public Technique
{
public:
    TransparentTechnique(Engine*, RenderGraph&);
    virtual ~TransparentTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::Transparent; }

    virtual void bindResources(RenderGraph&) override final {};
    virtual void render(RenderGraph&, Engine*) override final {};

private:

    GraphicsPipelineDescription mPipelineDescription;
    TaskID mTaskID;
};




#endif
