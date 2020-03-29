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

	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override final;

    virtual void bindResources(RenderGraph& graph) override final
	{
		graph.bindImage(kDFGLUT, mDFGLUTView);
	}

private:

	ComputePipelineDescription mPipelineDesc;
	TaskID mTaskID;

	Image mDFGLUT;
	ImageView mDFGLUTView;

	bool mFirstFrame;
};

#endif
