#include "Engine/Camera.hpp"

Frustum::Frustum(const float4& position,
                 const float4& direction,
                 const float lenght,
                 const float angle)
{
 // TODO
}


bool Frustum::isContainedWithin(const float4 &point) const
{
    bool inFrontOf = true;

    inFrontOf &= mNearPlane.isInFrontOf(point);
    inFrontOf &= mFarPlane.isInFrontOf(point);
    inFrontOf &= mLeftPlane.isInFrontOf(point);
    inFrontOf &= mRightPLane.isInFrontOf(point);
    inFrontOf &= mTopPlane.isInFrontOf(point);
    inFrontOf &= mBottomPlane.isInFrontOf(point);

    return inFrontOf;
}


bool Frustum::isContainedWithin(const float3 &point) const
{
    return isContainedWithin({point, 0.0f});
}


bool Frustum::isContainedWithin(const AABB &aabb, const EstimationMode mode) const
{
    bool inFrontOf = true;

    inFrontOf &= mNearPlane.isInFrontOf(aabb, mode);
    inFrontOf &= mFarPlane.isInFrontOf(aabb, mode);
    inFrontOf &= mLeftPlane.isInFrontOf(aabb, mode);
    inFrontOf &= mRightPLane.isInFrontOf(aabb, mode);
    inFrontOf &= mTopPlane.isInFrontOf(aabb, mode);
    inFrontOf &= mBottomPlane.isInFrontOf(aabb, mode);

    return inFrontOf;
}
