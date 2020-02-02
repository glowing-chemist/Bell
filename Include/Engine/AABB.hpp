#ifndef AABB_HPP
#define AABB_HPP

#include <array>

#include "GeomUtils.h"


// 8 float4 representing the 8 verticies of a cude
struct Cube
{
    // Verticies going clockwise looking down (perpendicular) to a single face
    float3 mUpper1; // Corresponds to TopFrontLeft
    float3 mUpper2;
    float3 mUpper3;
    float3 mUpper4;

    float3 mLower1;
    float3 mLower2;
    float3 mLower3; // Corresponds to BottomBackRight
    float3 mLower4;
};


// An axis aligned bounding box, using the vulkan corrdinate system (origin in the top left corner).
class AABB
{
public:
    AABB(const float3& diagonalTop, const float3& diagonalBottom) :
        mMinimum{diagonalTop},
        mMaximum{diagonalBottom} {}

    AABB(const Cube& cube) :
        mMinimum{cube.mUpper1},
        mMaximum{cube.mLower3} {}

	// Allow a zero sized AABB to be constructed
	AABB() = default;

    Cube getCube() const;
    std::array<float3, 8> getCubeAsVertexArray() const;

    // std::limits<float>::max() to indicate no intersection
    float intersectionDistance(const Ray&) const;

	bool contains(const float3&) const;
    bool contains(const AABB&, const EstimationMode) const;

    AABB& operator*=(const glm::mat4&);

    AABB& operator*=(const float3&);
    AABB& operator+=(const float3&);
    AABB& operator-=(const float3&);

	AABB operator*(const glm::mat4&);

	AABB operator*(const float3&);
	AABB operator+(const float3&);
	AABB operator-(const float3&);

    const float3& getTop() const
    { return mMaximum; }

    const float3& getBottom() const
    { return mMinimum; }

private:

    float3 mMinimum;
    float3 mMaximum;

};


#endif
