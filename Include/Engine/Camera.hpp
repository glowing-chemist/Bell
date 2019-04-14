#ifndef CAMERA_HPP
#define CAMERA_HPP


#include <array>

#include "GeomUtils.h"


class Frustum
{
public:

    Frustum(const float4& position, const float4& direction, const float lenght, const float angle);

    bool isContainedWithin(const float4& point) const;
    bool isContainedWithin(const float3& point) const;

    bool isContainedWithin(const AABB& aabb, const EstimationMode) const;

private:

    Plane mNearPlane;
    Plane mFarPlane;
    Plane mLeftPlane;
    Plane mRightPLane;
    Plane mTopPlane;
    Plane mBottomPlane;
};


class Camera
{
public:

    Camera(const float4& position,
           const float4& direction,
           float nearPlaneDistance = 0.1f,
           float farPlaneDistance = 100.0f,
           float fieldOfView = 90.0f);

    void moveForwards(const float distance);
    void moveLeft(const float distance);
    void moveRight(const float distance);

    void rotatePitch(const float);
    void rotateYaw(const float);

    float4 getPosition() const
        { return mPosition; }

    float4 getDirection() const
        { return mDirection; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getPerspectiveMatrix() const;

    Frustum getFrustum() const;

private:

    // Get the vector perpendicular to the direction vector (rotated 90 degrees clockwise)
    float4 rightDirectionVector() const
        { return float4{mDirection.z, 0.0f, -mDirection.x, 0.0f}; }

    float4 mPosition;
    float4 mDirection;
    float mNearPlaneDistance;
    float mFarPlaneDistance;
    float mFieldOfView;
};


#endif
