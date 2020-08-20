#include "Engine/Camera.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Frustum::Frustum(const float4x4 mvp)
{
    const float* mvpp = glm::value_ptr(mvp);
    // Right clipping plane.
    mRightPLane = float4( mvpp[3]-mvpp[0], mvpp[7]-mvpp[4], mvpp[11]-mvpp[8], mvpp[15]-mvpp[12] );
    // Left clipping plane.
     mLeftPlane = float4( mvpp[3]+mvpp[0], mvpp[7]+mvpp[4], mvpp[11]+mvpp[8], mvpp[15]+mvpp[12] );
    // Bottom clipping plane.
    mBottomPlane = float4( mvpp[3]+mvpp[1], mvpp[7]+mvpp[5], mvpp[11]+mvpp[9], mvpp[15]+mvpp[13] );
    // Top clipping plane.
    mTopPlane = float4( mvpp[3]-mvpp[1], mvpp[7]-mvpp[5], mvpp[11]-mvpp[9], mvpp[15]-mvpp[13] );
    // Far clipping plane.
    mFarPlane = float4( mvpp[3]-mvpp[2], mvpp[7]-mvpp[6], mvpp[11]-mvpp[10], mvpp[15]-mvpp[14] );
    // Near clipping plane.
    mNearPlane = float4( mvpp[3]+mvpp[2], mvpp[7]+mvpp[6], mvpp[11]+mvpp[10], mvpp[15]+mvpp[14] );
}


bool Frustum::isContainedWithin(const float4 &point) const
{
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
    return isContainedWithin({point, 0.0f});
}


bool Frustum::isContainedWithin(const AABB &aabb, const EstimationMode mode) const
{
    bool inFrontOf = true;

    inFrontOf = inFrontOf && mNearPlane.isInFrontOf(aabb, mode);
    inFrontOf = inFrontOf && mFarPlane.isInFrontOf(aabb, mode);
    inFrontOf = inFrontOf && mLeftPlane.isInFrontOf(aabb, mode);
    inFrontOf = inFrontOf && mRightPLane.isInFrontOf(aabb, mode);
    inFrontOf = inFrontOf && mTopPlane.isInFrontOf(aabb, mode);
    inFrontOf = inFrontOf && mBottomPlane.isInFrontOf(aabb, mode);

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
    if(mMode == CameraMode::ReversePerspective)
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
        return glm::perspective(glm::radians(mFieldOfView), mAspect, mNearPlaneDistance, mFarPlaneDistance);
    }
    else if(mMode == CameraMode::Orthographic)
    {
        return glm::ortho(-mFrameBufferSize.x / 2.0f, mFrameBufferSize.x / 2.0f,
                          -mFrameBufferSize.y / 2.0f, mFrameBufferSize.y / 2.0f, mNearPlaneDistance, mFarPlaneDistance);
    }

    BELL_TRAP;
}


Frustum Camera::getFrustum() const
{
    return Frustum{ getProjectionMatrix() * getViewMatrix() };
}
