#include "Engine/Camera.hpp"
#include "Engine/AABB.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Frustum::Frustum(const float4x4 mvp)
{
    const float* mvpp = glm::value_ptr(mvp);
    // Right clipping plane.
    mRightPLane = float4(mvpp[3]-mvpp[0], mvpp[7]-mvpp[4], mvpp[11]-mvpp[8], mvpp[15]-mvpp[12] );
    // Left clipping plane.
    mLeftPlane = float4(mvpp[3]+mvpp[0], mvpp[7]+mvpp[4], mvpp[11]+mvpp[8], mvpp[15]+mvpp[12] );
    // Bottom clipping plane.
    mBottomPlane = float4(mvpp[3]+mvpp[1], mvpp[7]+mvpp[5], mvpp[11]+mvpp[9], mvpp[15]+mvpp[13] );
    // Top clipping plane.
    mTopPlane = float4(mvpp[3]-mvpp[1], mvpp[7]-mvpp[5], mvpp[11]-mvpp[9], mvpp[15]-mvpp[13] );
    // Far clipping plane.
    mFarPlane = float4(mvpp[3]-mvpp[2], mvpp[7]-mvpp[6], mvpp[11]-mvpp[10], mvpp[15]-mvpp[14] );
    // Near clipping plane.
    mNearPlane = float4(mvpp[3]+mvpp[2], mvpp[7]+mvpp[6], mvpp[11]+mvpp[10], mvpp[15]+mvpp[14] );
}


bool Frustum::isContainedWithin(const float4 &point) const
{
    BELL_ASSERT(point.w == 1.0f, "Not a point")
    bool inFrontOf = true;

    inFrontOf = inFrontOf && mNearPlane.isInFrontOf(point);
    inFrontOf = inFrontOf && mFarPlane.isInFrontOf(point);
    inFrontOf = inFrontOf && mLeftPlane.isInFrontOf(point);
    inFrontOf = inFrontOf && mRightPLane.isInFrontOf(point);
    inFrontOf = inFrontOf && mTopPlane.isInFrontOf(point);
    inFrontOf = inFrontOf && mBottomPlane.isInFrontOf(point);

    return inFrontOf;
}


bool Frustum::isContainedWithin(const float3 &point) const
{
    return isContainedWithin({point, 1.0f});
}


Intersection Frustum::isContainedWithin(const AABB &aabb) const
{
    Intersection inFrontOf = static_cast<Intersection>(Intersection::Partial | Intersection::Contains);

    inFrontOf = static_cast<Intersection>(inFrontOf & mNearPlane.isInFrontOf(aabb));
    inFrontOf = static_cast<Intersection>(inFrontOf & mFarPlane.isInFrontOf(aabb));
    inFrontOf = static_cast<Intersection>(inFrontOf & mLeftPlane.isInFrontOf(aabb));
    inFrontOf = static_cast<Intersection>(inFrontOf & mRightPLane.isInFrontOf(aabb));
    inFrontOf = static_cast<Intersection>(inFrontOf & mTopPlane.isInFrontOf(aabb));
    inFrontOf = static_cast<Intersection>(inFrontOf & mBottomPlane.isInFrontOf(aabb));

    return inFrontOf;
}


void Camera::moveForward(const float distance)
{
    mPosition += distance * mDirection;
}


void Camera::moveBackward(const float distance)
{
    mPosition -= distance * mDirection;
}


void Camera::moveLeft(const float distance)
{
    mPosition -= distance * getRight();
}


void Camera::moveRight(const float distance)
{
    mPosition += distance * getRight();
}


void Camera::moveUp(const float distance)
{
	mPosition += mUp * distance;
}


void Camera::moveDown(const float distance)
{
	mPosition -= mUp * distance;
}


void Camera::rotatePitch(const float angle)
{
    const float3 rotationAxis = getRight();
    const glm::mat3 rotation = glm::rotate(glm::radians(angle), rotationAxis);
    mDirection = glm::normalize(rotation * mDirection);
    mUp = glm::normalize(rotation * mUp);
}


void Camera::rotateYaw(const float angle)
{
    const glm::mat3 rotation = glm::rotate(glm::radians(angle), mUp);
    mDirection = glm::normalize(rotation * mDirection);
}


void Camera::rotateWorldUp(const float angle)
{
    const glm::mat3 rotation = glm::rotate(glm::radians(angle), float3(0.0f, -1.0f, 0.0f));
    mDirection = glm::normalize(rotation * mDirection);
    mUp = glm::normalize(rotation * mUp);
}


float4x4 Camera::getViewMatrix() const
{
	return glm::lookAt(mPosition, mPosition + mDirection, mUp);
}


float4x4 Camera::getProjectionMatrix() const
{
    if(mMode == CameraMode::InfinitePerspective)
    {
        float fov = 1.0f / tan(glm::radians(mFieldOfView) / 2.0f);
        return glm::mat4(
            fov / mAspect, 0.0f, 0.0f, 0.0f,
            0.0f, fov, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, mNearPlaneDistance, 0.0f);
    }
    else if(mMode == CameraMode::Perspective)
    {
        return glm::perspective(glm::radians(mFieldOfView), mAspect, mFarPlaneDistance, mNearPlaneDistance);
    }
    else if(mMode == CameraMode::Orthographic)
    {
        return glm::ortho(-mOrthographicSize.x / 2.0f, mOrthographicSize.x / 2.0f,
                          -mOrthographicSize.y / 2.0f, mOrthographicSize.y / 2.0f, mNearPlaneDistance, mFarPlaneDistance);
    }

    BELL_TRAP;

    return float4x4();
}


float4x4 Camera::getProjectionMatrixOverride(const CameraMode overrideMode) const
{
    if(overrideMode == CameraMode::InfinitePerspective)
    {
        float fov = 1.0f / tan(glm::radians(mFieldOfView) / 2.0f);
        return glm::mat4(
            fov / mAspect, 0.0f, 0.0f, 0.0f,
            0.0f, fov, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, mNearPlaneDistance, 0.0f);
    }
    else if(overrideMode == CameraMode::Perspective)
    {
        return glm::perspective(glm::radians(mFieldOfView), mAspect, mFarPlaneDistance, mNearPlaneDistance);
    }
    else if(overrideMode == CameraMode::Orthographic)
    {
        return glm::ortho(-mOrthographicSize.x / 2.0f, mOrthographicSize.x / 2.0f,
                          -mOrthographicSize.y / 2.0f, mOrthographicSize.y / 2.0f, mNearPlaneDistance, mFarPlaneDistance);
    }

    BELL_TRAP;

    return float4x4(1.0f);
}


Frustum Camera::getFrustum() const
{
    return Frustum{ getProjectionMatrix() * getViewMatrix() };
}


AABB Camera::getFrustumAABB() const
{
    const float4x4 viewProj = getProjectionMatrixOverride(CameraMode::Perspective) * getViewMatrix();
    const float4x4 invViewProj = glm::inverse(viewProj);

    const std::array<float4, 8> clipSpaceCorners{float4{1.0f, 1.0f, 1.0f, 1.0f}, float4{-1.0f, 1.0f, 1.0f, 1.0f},
                                                 float4{1.0f, -1.0f, 1.0f, 1.0f}, float4{1.0f, 1.0f, -1.0f, 1.0f},
                                                 float4{-1.0f, -1.0f, 1.0f, 1.0f}, float4{-1.0f, 1.0f, -1.0f, 1.0f},
                                                 float4{-1.0f, -1.0f, -1.0f, 1.0f}, float4{1.0f, -1.0f, -1.0f, 1.0f}};

    float4 min = float4{INFINITY, INFINITY, INFINITY, INFINITY};
    float4 max = float4{-INFINITY, -INFINITY, -INFINITY, -INFINITY};
    for(const float4& corner : clipSpaceCorners)
    {
        float4 transformedCorner = invViewProj * corner;
        transformedCorner /= transformedCorner.w;

        min = componentWiseMin(min, transformedCorner);
        max = componentWiseMax(max, transformedCorner);
    }

    return AABB{min, max};
}
