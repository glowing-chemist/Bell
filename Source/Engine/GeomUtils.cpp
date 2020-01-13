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
    const auto centreToPoint = glm::normalize(point - mCenterPosition);
    const float angle = glm::dot(centreToPoint, mNormal);

    return angle > 0.0f;
}


float Plane::distanceTo(const float3& point) const
{
    const float3 centerToPoint = point - mCenterPosition;
    const float angle = 90.0f - std::acos(glm::dot(mNormal, centerToPoint));

    return glm::length(centerToPoint) * std::sin(angle);
}


float3 Plane::closestPoint(const float3& point) const
{
    const float distancetoPoint = distanceTo(point);

    return point - (distancetoPoint * mNormal);
}
