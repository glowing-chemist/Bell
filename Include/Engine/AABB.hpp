#ifndef AABB_HPP
#define AABB_HPP

#include <array>

#include "GeomUtils.h"


// 8 float4 representing the 8 verticies of a cude
struct Cube
{
    // Verticies going clockwise looking down (perpendicular) on a single face
    float4 mUpper1;
    float4 mUpper2;
    float4 mUpper3;
    float4 mUpper4;

    float4 mLower1;
    float4 mLower2;
    float4 mLower3;
    float4 mLower4;
};


class AABB
{
public:
    AABB(const glm::vec4& diagonalTop, const glm::vec4& diagonalBottom);
    AABB(const Cube&);

    Cube getCube() const;
    std::array<float4, 8> getCubeAsArray() const;

    AABB& operator*(const glm::mat4&);

    AABB& operator*(const glm::vec4&);
    AABB& operator+(const glm::vec4&);
    AABB& operator-(const glm::vec4&);

private:

    float4 mTopBackLeft;
    float4 mBottomFrontRight;

};


#endif
