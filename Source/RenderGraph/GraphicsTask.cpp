#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

#include <limits>


std::vector<ClearValues> GraphicsTask::getClearValues() const
{
    std::vector<ClearValues> clearValues;

    for(const auto& attatchment : mOutputAttachments)
    {
        switch(attatchment.mLoadOp)
        {
            case LoadOp::Clear_Black:
                clearValues.emplace_back(attatchment.mType, 0.0f, 0.0f, 0.0f, 0.0f);
                break;

            case LoadOp::Clear_White:
                clearValues.emplace_back(attatchment.mType, 1.0f, 1.0f, 1.0f, 1.0f);
                break;

            case LoadOp::Clear_ColourBlack_AlphaWhite:
                clearValues.emplace_back(attatchment.mType, 0.0f, 0.0f, 0.0f, 1.0f);
                break;

            case LoadOp::Clear_Float_Max:
                clearValues.emplace_back(attatchment.mType, std::numeric_limits<float>::max(),
                                                            std::numeric_limits<float>::max(),
                                                            std::numeric_limits<float>::max(),
                                                            std::numeric_limits<float>::max());
                break;

            default:
                clearValues.emplace_back(attatchment.mType, 0.0f, 0.0f, 0.0f, 1.0f);
                break;
        }
    }

    return clearValues;
}


namespace std
{

    size_t hash<GraphicsPipelineDescription>::operator()(const GraphicsPipelineDescription& desc) const noexcept
    {
        std::hash<std::string> stringHasher{};

        size_t hash = 0;
		hash += stringHasher(desc.mVertexShader->getFilePath());

        if(desc.mGeometryShader)
			hash += stringHasher((*desc.mGeometryShader)->getFilePath());

        if(desc.mHullShader)
			hash += stringHasher((*desc.mHullShader)->getFilePath());

        if(desc.mTesselationControlShader)
			hash += stringHasher((*desc.mTesselationControlShader)->getFilePath());

		hash += stringHasher(desc.mFragmentShader->getFilePath());

		if (desc.mDepthWrite)
			hash = ~hash;

        hash += desc.mViewport.x;
        hash += desc.mViewport.y;
        hash += desc.mScissorRect.x;
        hash += desc.mScissorRect.y;

        return hash;
    }

}


bool operator==(const GraphicsTask& lhs, const GraphicsTask& rhs)
{
    std::hash<GraphicsPipelineDescription> hasher{};
    return hasher(lhs.getPipelineDescription()) == hasher(rhs.getPipelineDescription());
}


bool operator==(const GraphicsPipelineDescription& lhs, const GraphicsPipelineDescription& rhs)
{
    std::hash<GraphicsPipelineDescription> hasher{};
    return hasher(lhs) == hasher(rhs);
}

