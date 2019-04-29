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
        mTopFrontLeft{diagonalTop},
        mBottomBackRight{diagonalBottom} {}

    AABB(const Cube& cube) :
        mTopFrontLeft{cube.mUpper1},
        mBottomBackRight{cube.mLower3} {}

	// Allow a zero sized AABB to be constructed
	AABB() = default;

    Cube getCube() const;
    std::array<float3, 8> getCubeAsVertexArray() const;

    // std::limits<float>::max() to indicate no intersection
    float intersectionDistance(const Ray&) const;

	bool contains(const float3&) const;
    bool contains(const AABB&, const EstimationMode) const;

    AABB& operator*(const glm::mat3&);

    AABB& operator*(const float3&);
    AABB& operator+(const float3&);
    AABB& operator-(const float3&);

private:

    float3 mTopFrontLeft;
    float3 mBottomBackRight;

};


#endif
