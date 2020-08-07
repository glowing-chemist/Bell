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
