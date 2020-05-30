#ifndef AABB_HPP
#define AABB_HPP

#include <array>

#include "GeomUtils.h"


// 8 float4 representing the 8 verticies of a cude
struct Cube
{
    // Verticies going clockwise looking down (perpendicular) to a single face
    float4 mUpper1; // Corresponds to TopFrontLeft
    float4 mUpper2;
    float4 mUpper3;
    float4 mUpper4;

    float4 mLower1;
    float4 mLower2;
    float4 mLower3; // Corresponds to BottomBackRight
    float4 mLower4;
};


// An axis aligned bounding box, using the vulkan corrdinate system (origin in the top left corner).
class AABB
{
public:
    AABB(const float4& diagonalTop, const float4& diagonalBottom) :
        mMinimum{diagonalTop},
        mMaximum{diagonalBottom} {}

    AABB(const Cube& cube) :
        mMinimum{cube.mUpper1},
        mMaximum{cube.mLower3} {}

	// Allow a zero sized AABB to be constructed
	AABB() = default;

    Cube getCube() const;
    std::array<float4, 8> getCubeAsVertexArray() const;

    // std::limits<float>::max() to indicate no intersection
    float intersectionDistance(const Ray&) const;

	bool contains(const float4&) const;
    bool contains(const AABB&, const EstimationMode) const;

    AABB& operator*=(const float4x4&);

    AABB& operator*=(const float4&);
    AABB& operator+=(const float4&);
    AABB& operator-=(const float4&);

	AABB operator*(const glm::mat4&) const;

	AABB operator*(const float4&) const;
	AABB operator+(const float4&) const;
	AABB operator-(const float4&) const;

    const float4& getTop() const
    { return mMaximum; }

    const float4& getBottom() const
    { return mMinimum; }

    float4 getCentralPoint() const
    { return mMinimum + (mMaximum - mMinimum) * 0.5f; }

    float3 getSideLengths() const
    {
        return glm::abs(mMaximum - mMinimum);
    }

private:

    float4 mMinimum;
    float4 mMaximum;

};


class OBB
{
public:
    OBB() = default;

    OBB(const Cube& cube) :
    mCube{cube} {}


    const Cube& getCube()
    {
	return mCube;
    }

    float4 getCentralPoint() const
    { return mCube.mLower3 + (mCube.mUpper1 - mCube.mLower3) * 0.5f; }

    float3 getSideLengths() const
    {
	return float3
	{
	    glm::length(mCube.mLower2 - mCube.mLower3),
	    glm::length(mCube.mLower1 - mCube.mUpper1),
	    glm::length(mCube.mLower3 - mCube.mLower4)
	};
    }

    OBB operator*(const float4x4&) const;
    OBB operator*(const float4&) const;
    OBB operator+(const float4&) const;
    OBB operator-(const float4&) const;

private:

    Cube mCube;

};


#endif
