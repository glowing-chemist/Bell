#include <cmath>

#include "Engine/GeomUtils.h"
#include "Engine/AABB.hpp"


Intersection Plane::isInFrontOf(const AABB& aabb) const
{
    const auto aabbVerticies = aabb.getCubeAsVertexArray();

    bool all = true;
    bool any = false;
    for(const auto& vertex : aabbVerticies)
    {
        const bool front = isInFrontOf(vertex);
        all = all && front;
        any = any || front;
    }

    return static_cast<Intersection>((any ? Intersection::Partial : Intersection::None) | (all ? Intersection::Contains : Intersection::None));
}


bool Plane::isInFrontOf(const float3& point) const
{
    return glm::dot(mPlane, float4(point, 1.0f)) > 0.0f;
}

