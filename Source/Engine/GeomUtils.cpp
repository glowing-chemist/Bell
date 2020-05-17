#include <cmath>

#include "Engine/GeomUtils.h"
#include "Engine/AABB.hpp"


bool Plane::isInFrontOf(const AABB& aabb, const EstimationMode estimationMode) const
{
    const auto aabbVerticies = aabb.getCubeAsVertexArray();
    bool inFrontOf = true;

    if(estimationMode == EstimationMode::Under)
    {
        for(const auto& vertex : aabbVerticies)
        {
            inFrontOf = inFrontOf && isInFrontOf(vertex);
        }
    }
    else
    {
        inFrontOf = false;
        for(const auto& vertex : aabbVerticies)
        {
            if (isInFrontOf(vertex))
            {
                inFrontOf = true;
                break;
            }
        }
    }

    return inFrontOf;
}


bool Plane::isInFrontOf(const float3& point) const
{
    return glm::dot(mPlane, float4(point, 1.0f)) > 0.0f;
}

