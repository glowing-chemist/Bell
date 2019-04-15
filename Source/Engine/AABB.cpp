#include "Engine/AABB.hpp"


std::array<float3, 8> AABB::getCubeAsVertexArray() const
{
    float3 upper1{mTopFrontLeft};
    float3 upper2{mTopFrontLeft.x, mTopFrontLeft.y, mBottomBackRight.z};
    float3 upper3{mBottomBackRight.x, mTopFrontLeft.y, mBottomBackRight.z};
    float3 upper4{mBottomBackRight.x, mTopFrontLeft.y, mTopFrontLeft.z};

    float3 lower1{mTopFrontLeft.x, mBottomBackRight.y, mTopFrontLeft.z};
    float3 lower2{mTopFrontLeft.x, mBottomBackRight.y, mBottomBackRight.z};
    float3 lower3{mBottomBackRight};
    float3 lower4{mBottomBackRight.x, mBottomBackRight.y, mTopFrontLeft.z};

    return {upper1, upper2, upper3, upper4,
            lower1, lower2, lower3, lower4};
}


Cube AABB::getCube() const
{

    auto verticies = getCubeAsVertexArray();

    return Cube{verticies[0], verticies[1], verticies[2], verticies[3],
                verticies[4], verticies[5], verticies[6], verticies[7]};
}


AABB& AABB::operator*(const glm::mat3& mat)
{
    mTopFrontLeft = mat * mTopFrontLeft;
    mBottomBackRight = mat * mBottomBackRight;

    return *this;
}


AABB& AABB::operator*(const float3& vec)
{
    mTopFrontLeft *= vec;
    mBottomBackRight *= vec;

    return *this;
}


AABB& AABB::operator+(const float3& vec)
{
    mTopFrontLeft += vec;
    mBottomBackRight += vec;

    return *this;
}


AABB& AABB::operator-(const float3& vec)
{
    mTopFrontLeft -= vec;
    mBottomBackRight -= vec;

    return *this;
}
