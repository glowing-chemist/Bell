#include "Engine/Camera.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

Frustum::Frustum(const float3& position,
                 const float3& direction,
                 const float3& up,
                 const float lenght,
                 const float startOffset,
                 const float angle)
{
    // calculating the near and far planes is relivitly simple as they have the same normal(or oposite) of
    // the cameras direction.
    mNearPlane = {position + (direction * startOffset), direction};
    mFarPlane = {position + (direction * lenght), -direction};

    // We construct the other four planes by taking a pont and a vector in the center of the frustum
    // and rotating it in the apropriate direction by half the field of view angle.
    // I'm sure theres probably a trick to do this more effiently but this will do for now.
	glm::mat3 leftPlaneRotation = glm::rotate(glm::mat4(1.0f), -(angle / 2.0f), up);
	glm::mat3 rightPlaneRotation = glm::rotate(glm::mat4(1.0f), (angle / 2.0f), up);
    float3 rightVector = glm::normalize(glm::cross(direction, up));

    mLeftPlane = {(position + (direction * (lenght / 2.0f))) * leftPlaneRotation,
                  rightVector * leftPlaneRotation};

    mRightPLane = {(position + (direction * (lenght / 2.0f))) * rightPlaneRotation,
                  -rightVector * rightPlaneRotation};


	glm::mat3 bottomPlaneRotation = glm::rotate(glm::mat4(1.0f), (angle / 2.0f), rightVector);
	glm::mat3 topPlaneRotation = glm::rotate(glm::mat4(1.0f), -(angle / 2.0f), rightVector);
    float3 normalisedUp = glm::normalize(up);

    mBottomPlane = {(position + (direction * (lenght / 2.0f))) * bottomPlaneRotation,
                    normalisedUp * rightPlaneRotation};

    mTopPlane = {(position + (direction * (lenght / 2.0f))) * topPlaneRotation,
                  -normalisedUp * leftPlaneRotation};
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
    mPosition -= distance * rightDirectionVector();
}


void Camera::moveRight(const float distance)
{
    mPosition += distance * rightDirectionVector();
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
	const float3 rotationAxis = rightDirectionVector();
	const glm::mat3 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), rotationAxis);
    mDirection = rotation * mDirection;
    mUp = rotation * mUp;
}


void Camera::rotateYaw(const float angle)
{
	const glm::mat3 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), mUp);
    mDirection = rotation * mDirection;
}


glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(mPosition, mPosition + mDirection, mUp);
}


glm::mat4 Camera::getPerspectiveMatrix() const
{
	return glm::perspective(glm::radians(mFieldOfView), 1.0f, mNearPlaneDistance, mFarPlaneDistance);
}


Frustum Camera::getFrustum() const
{
    return Frustum{ mPosition,
                    mDirection,
                    mUp,
                    mFarPlaneDistance,
                    mNearPlaneDistance,
                    mFieldOfView };
}
