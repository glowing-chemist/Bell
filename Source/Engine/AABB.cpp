#include "Engine/AABB.hpp"

#include <limits>

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


float AABB::intersectionDistance(const Ray& ray) const
{
    float3 rayDirection = ray.getDirection();
    float3 rayOrigin = ray.getPosition();
    float3 inverseDirection{1.0f / rayDirection.x, 1.0f / rayDirection.y, 1.0f / rayDirection.z};

    // mTopFrontLeft is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
    // r.org is origin of ray
    float t1 = (mTopFrontLeft.x - rayOrigin.x)*inverseDirection.x;
    float t2 = (mBottomBackRight.x - rayOrigin.x)*inverseDirection.x;
    float t3 = (mTopFrontLeft.y - rayOrigin.y)*inverseDirection.y;
    float t4 = (mBottomBackRight.y - rayOrigin.y)*inverseDirection.y;
    float t5 = (mTopFrontLeft.z - rayOrigin.z)*inverseDirection.z;
    float t6 = (mBottomBackRight.z - rayOrigin.z)*inverseDirection.z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    // if tmax < 0, ray the ray intersects AABB, but the AABB is behind the ray (direction faces away)
    // or no intersection at all.
    if (tmax < 0 || tmin > tmax)
    {
        return std::numeric_limits<float>::max();
    }

    return tmin;
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
