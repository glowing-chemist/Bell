#ifndef CAMERA_HPP
#define CAMERA_HPP


#include <array>

#include "GeomUtils.h"


class Frustum
{
public:

    Frustum(const float3& position,
            const float3& direction,
            const float3& up,
            const float lenght,
            const float startOffset,
            const float angle);

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

    Camera(const float3& position,
           const float3& direction,
           float nearPlaneDistance = 0.1f,
           float farPlaneDistance = 100.0f,
           float fieldOfView = 90.0f)
        : mPosition{position},
          mDirection{direction},
          mUp{0.0f, -1.0f, 0.0f},
          mNearPlaneDistance{nearPlaneDistance},
          mFarPlaneDistance{farPlaneDistance},
          mFieldOfView{fieldOfView} {}


    void moveForward(const float distance);
    void moveBackward(const float distance);
    void moveLeft(const float distance);
    void moveRight(const float distance);

    void rotatePitch(const float);
    void rotateYaw(const float);

    float3 getPosition() const
        { return mPosition; }

    float3 getDirection() const
        { return mDirection; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getPerspectiveMatrix() const;

    Frustum getFrustum() const;

private:

    // Get the vector perpendicular to the direction vector (rotated 90 degrees clockwise)
    float3 rightDirectionVector() const
        { return glm::cross(mDirection, mUp); }

    float3 mPosition;
    float3 mDirection;
    float3 mUp;
    float mNearPlaneDistance;
    float mFarPlaneDistance;
    float mFieldOfView;
};


#endif
