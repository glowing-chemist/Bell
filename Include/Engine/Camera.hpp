#ifndef CAMERA_HPP
#define CAMERA_HPP


#include <array>

#include "GeomUtils.h"


class Frustum
{
public:

    Frustum(const float4x4 mvp);

    bool isContainedWithin(const float4& point) const;
    bool isContainedWithin(const float3& point) const;

    Intersection isContainedWithin(const AABB& aabb) const;

private:

    Plane mNearPlane;
    Plane mFarPlane;
    Plane mLeftPlane;
    Plane mRightPLane;
    Plane mTopPlane;
    Plane mBottomPlane;
};


enum class CameraMode
{
    InfinitePerspective = 0,
    Perspective,
    Orthographic
};


class Camera
{
public:

    Camera(const float3& position,
        const float3& direction,
        const float aspect,
        float nearPlaneDistance = 0.1f,
        float farPlaneDistance = 10.0f,
        float fieldOfView = 90.0f,
        const CameraMode mode = CameraMode::InfinitePerspective)
        :   mMode(mode),
            mPosition{ position },
            mDirection{ direction },
            mUp{ 0.0f, -1.0f, 0.0f },
            mAspect{ aspect },
            mNearPlaneDistance{nearPlaneDistance},
            mFarPlaneDistance{farPlaneDistance},
            mFieldOfView{fieldOfView} {}


    void moveForward(const float distance);
    void moveBackward(const float distance);
    void moveLeft(const float distance);
    void moveRight(const float distance);
    void moveUp(const float distance);
    void moveDown(const float distance);

    void rotatePitch(const float);
    void rotateYaw(const float);
    void rotateWorldUp(const float);

    void setMode(const CameraMode mode)
    {
        mMode = mode;
    }

    CameraMode getMode() const
    {
        return mMode;
    }

    void setOrthographicSize(const float2 size)
        { mOrthographicSize = size; }

    const float2& getOrthographicSize() const
    {
        return mOrthographicSize;
    }

    const float3& getPosition() const
        { return mPosition; }

	void setPosition(const float3& pos)
		{ mPosition = pos; }

    const float3& getDirection() const
        { return mDirection; }

	void setDirection(const float3& dir)
		{ mDirection = dir; }

    void setUp(const float3& up)
        { mUp = up; }

    const float3 getUp() const
        { return mUp; }

    // Get the vector perpendicular to the direction vector (rotated 90 degrees clockwise)
    float3 getRight() const
    { return glm::cross(glm::normalize(mDirection), mUp); }

    void setNearPlane(const float nearDistance)
        { mNearPlaneDistance = nearDistance; }

    void setFarPlane(const float farDistance)
        { mFarPlaneDistance = farDistance; }

    void setFOVDegrees(const float fov)
        { mFieldOfView = fov; }

    void setAspect(const float aspect)
        { mAspect = aspect; }

    float getAspect() const
    { return mAspect; }

    float4x4 getViewMatrix() const;
    float4x4 getProjectionMatrix() const;
    float4x4 getProjectionMatrixOverride(const CameraMode) const;

	float getNearPlane() const
	{ return mNearPlaneDistance; }

	float getFarPlane() const
	{ return mFarPlaneDistance; }

    float getFOV() const
    { return mFieldOfView; }

    Frustum getFrustum() const;

	AABB getFrustumAABB() const;


private:

    CameraMode mMode;
    float2 mOrthographicSize;

    float3 mPosition;
    float3 mDirection;
    float3 mUp;
    float mAspect;
    float mNearPlaneDistance;
    float mFarPlaneDistance;
    float mFieldOfView;
};


#endif
