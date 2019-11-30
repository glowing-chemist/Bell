#ifndef LIGHT_FROXELATION_TECHNIQUE_HPP
#define LIGHT_FROXELATION_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "RenderGraph/ComputeTask.hpp"

class LightFroxelationTechnique : public Technique
{
public:
	LightFroxelationTechnique(Engine*);
	~LightFroxelationTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::LightFroxelation;
	}

	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&);

	virtual void bindResources(RenderGraph&) const;

private:

	ComputePipelineDescription mActiveFroxelsDesc;
	ComputeTask				   mActiveFroxels;

	Image					   mActiveFroxelsImage;
	ImageView				   mActiveFroxelsImageView;
};

#endif