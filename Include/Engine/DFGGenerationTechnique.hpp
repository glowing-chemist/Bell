#ifndef DFGGENERATION_TECHNIQUE_HPP
#define DFGGENERATION_TECHNIQUE_HPP

#include "Technique.hpp"
#include "DefaultResourceSlots.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"


class DFGGenerationTechnique : public Technique
{
public:

	DFGGenerationTechnique(Engine*, RenderGraph& graph);
	virtual ~DFGGenerationTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DFGGeneration;
	}

	virtual void render(RenderGraph&, Engine*) override final;

    virtual void bindResources(RenderGraph& graph) override final
	{
        if(!graph.isResourceSlotBound(kDFGLUT))
        {
            graph.bindImage(kDFGLUT, mDFGLUTView);
        }
    }

private:

    Shader mDFGGenerationShader;
	TaskID mTaskID;

	Image mDFGLUT;
	ImageView mDFGLUTView;

	bool mFirstFrame;
};

#endif
