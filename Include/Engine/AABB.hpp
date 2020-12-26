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
    Intersection contains(const AABB&) const;

    AABB& operator*=(const float4x4&);

    AABB& operator*=(const float4&);
    AABB& operator+=(const float4&);
    AABB& operator-=(const float4&);

	AABB operator*(const glm::mat4&) const;

	AABB operator*(const float4&) const;
	AABB operator+(const float4&) const;
	AABB operator-(const float4&) const;

    const float4& getMax() const
    { return mMaximum; }

    const float4& getMin() const
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


struct Basis
{
    float3 mX;
    float3 mY;
    float3 mZ;
};


class OBB
{
public:
    OBB() = default;

    OBB(const Basis& basis, const float3& half, const float3& start) :
    mBasis{basis}, mHalfSize{half}, mStart{start} {}

    void setStart(const float3& start)
    {
        mStart = start;
    }

    void setSideLenghts(const float3& len)
    {
        mHalfSize = len / 2.0f;
    }

    void setBasisVectors(const Basis& basis)
    {
        mBasis = basis;
    }

    float4 getCentralPoint() const
    { return float4(mStart + (mBasis.mX * mHalfSize.x + mBasis.mY * mHalfSize.y  + mBasis.mZ * mHalfSize.z), 1.0f); }

    float3 getSideLengths() const
    {
        return mHalfSize * 2.0f;
    }

    const float3& getHalfSize() const
    {
        return mHalfSize;
    }

    const float3& getStart() const
    {
        return mStart;
    }

    const Basis& getBasisVectors() const
    {
        return mBasis;
    }

    bool intersects(const OBB&) const;

    OBB operator*(const float4x4&) const;
    OBB operator*(const float4&) const;
    OBB operator+(const float4&) const;
    OBB operator-(const float4&) const;

private:

    Basis mBasis;
    float3 mHalfSize;
    float3 mStart;
};


#endif
