#include "Engine/Camera.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

Frustum::Frustum(const float3& position,
                 const float3& direction,
                 const float3& up,
                 const float lenght,
                 const float startOffset,
                 const float angle,
                 const float aspect)
{
    // calculating the near and far planes is relivitly simple as they have the same normal(or oposite) of
    // the cameras direction.
    mNearPlane = {position + (direction * startOffset), direction};
    mFarPlane = {position + (direction * lenght), -direction};

    // We construct the other four planes by taking a pont and a vector in the center of the frustum
    // and rotating it in the apropriate direction by half the field of view angle.
    // I'm sure theres probably a trick to do this more effiently but this will do for now.
	glm::mat3 leftPlaneRotation = glm::rotate(glm::mat4(1.0f), -(angle / 2.0f) * aspect, up);
	glm::mat3 rightPlaneRotation = glm::rotate(glm::mat4(1.0f), (angle / 2.0f) * aspect, up);
    float3 rightVector = glm::normalize(glm::cross(direction, up));

    mLeftPlane = {(leftPlaneRotation * direction) * (lenght / 2.0f) + position,
                   (leftPlaneRotation * rightVector)};

    mRightPLane = {(rightPlaneRotation * direction) * (lenght / 2.0f) + position,
                   -rightPlaneRotation * rightVector};


	glm::mat3 bottomPlaneRotation = glm::rotate(glm::mat4(1.0f), (angle / 2.0f), rightVector);
	glm::mat3 topPlaneRotation = glm::rotate(glm::mat4(1.0f), -(angle / 2.0f), rightVector);
    float3 normalisedUp = glm::normalize(up);

    mBottomPlane = { (bottomPlaneRotation * direction) * (lenght / 2.0f) + position,
                    -(bottomPlaneRotation * normalisedUp)};

    mTopPlane = { (topPlaneRotation * direction) * (lenght / 2.0f) + position,
                   topPlaneRotation * normalisedUp};
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
}


float4x4 Camera::getViewMatrix() const
{
	return glm::lookAt(mPosition, mPosition + mDirection, mUp);
}


float4x4 Camera::getPerspectiveMatrix() const
{
    float fov = 1.0f / tan(glm::radians(mFieldOfView) / 2.0f);
    return glm::mat4(
        fov / mAspect, 0.0f, 0.0f, 0.0f,
        0.0f, fov, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, mNearPlaneDistance, 0.0f);
}


float4x4 Camera::getOrthographicsMatrix() const
{
    return glm::ortho(0.0f, 500.0f, 0.0f, 500.0f, 1.0f, -1.0f);
}


Frustum Camera::getFrustum() const
{
    return Frustum{ mPosition,
                    mDirection,
                    mUp,
                    mFarPlaneDistance,
                    mNearPlaneDistance,
                    mFieldOfView,
                    mAspect };
}
