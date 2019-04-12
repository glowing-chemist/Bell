#include <cmath>

#include "Engine/GeomUtils.h"
#include "Engine/AABB.hpp"


bool Plane::isInFrontOf(const AABB& aabb, const Estimation estimationMode) const
{
    const auto aabbVerticies = aabb.getCubeAsArray();
    bool inFrontOf = true;

    if(estimationMode == Estimation::Over)
    {
        for(const auto& vertex : aabbVerticies)
        {
            inFrontOf &= isInFrontOf(vertex);
        }
    }
    else
    {
        for(const auto& vertex : aabbVerticies)
        {
            inFrontOf &= isInFrontOf(vertex);

            if(inFrontOf)
                break;
        }
    }

    return inFrontOf;
}


bool Plane::isInFrontOf(const float3& point) const
{
    return isInFrontOf(float4{point, 1.0f});
}


bool Plane::isInFrontOf(const float4& point) const
{
    const auto centreToPoint = glm::normalize(point - mCenterPosition);
    const float angle = glm::dot(centreToPoint, mNormal);

    return angle > 0.0f;
}


float Plane::distanceTo(const float3& point) const
{
    return distanceTo(float4{point, 1.0f});
}


float Plane::distanceTo(const float4& point) const
{
    const float4 centerToPoint = point - mCenterPosition;
    const float angle = 90.0f - std::acos(glm::dot(mNormal, centerToPoint));

    return glm::length(centerToPoint) * std::sin(angle);
}


float4 Plane::closestPoint(const float3& point) const
{
    return closestPoint({point, 1.0f});
}


float4 Plane::closestPoint(const float4& point) const
{
    const float distancetoPoint = distanceTo(point);

    return point - (distancetoPoint * mNormal);
}
