#ifndef TAA_TECHNIQUE_HPP
#define TAA_TECHNIQUE_HPP

#include "Engine/Technique.hpp"

#include "RenderGraph/ComputeTask.hpp"

#include "Core/Image.hpp"



class TAATechnique : public Technique
{
public:
	TAATechnique(RenderEngine*, RenderGraph&);

	virtual PassType getPassType() const override
	{
		return PassType::TAA;
	}

	virtual void render(RenderGraph&, RenderEngine*) override;

	virtual void bindResources(RenderGraph&) override;

private:

	// Ping-Pong these every other frame
	Image mHistoryImage;
	ImageView mHistoryImageView;
	Image mNextHistoryImage;
	ImageView mNextHistoryImageView;

    Sampler mTAASAmpler;

    Shader                     mTAAShader;
	TaskID					   mTaskID;

	bool mFirstFrame;// need to perform some setup;
};

#endif
