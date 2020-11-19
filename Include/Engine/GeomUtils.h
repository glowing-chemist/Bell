#ifndef GEOMUTILS_HPP
#define GEOMUTILS_HPP

#include "Core/BellLogging.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>

#include <algorithm>


using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using uint2 = glm::vec<2, uint32_t>;
using uint3 = glm::vec<3, uint32_t>;
using uint4 = glm::vec<4, uint32_t>;

using int2 = glm::vec<2, int32_t>;
using int3 = glm::vec<3, int32_t>;
using int4 = glm::vec<4, int32_t>;

using short2 = glm::vec<2, int16_t>;
using short3 = glm::vec<3, int16_t>;
using short4 = glm::vec<4, int16_t>;

using ushort2 = glm::vec<2, uint16_t>;
using ushort3 = glm::vec<3, uint16_t>;
using ushort4 = glm::vec<4, uint16_t>;

using char2 = glm::vec<2, int8_t>;
using char3 = glm::vec<3, int8_t>;
using char4 = glm::vec<4, int8_t>;

using uchar2 = glm::vec<2, uint8_t>;
using uchar3 = glm::vec<3, uint8_t>;
using uchar4 = glm::vec<4, uint8_t>;

using float4x4 = glm::mat4;
using float3x3 = glm::mat3;
using float4x3 = glm::mat4x3;
using float3x4 = glm::mat3x4;

class AABB;


enum Intersection : uint8_t
{
    None = 0,
    Contains = 1 << 1,
    Partial = 1 << 2,
};

class Plane
{
	public:
	
    Plane(const float4& plane) : mPlane{plane} { glm::normalize(mPlane); }
    Plane() = default;

    Intersection isInFrontOf(const AABB&) const;
    bool isInFrontOf(const float3&) const;

	private:

    float4 mPlane; // x,y,z contain normal, w contains distance to origin along normal.
};


class Ray
{
public:
    Ray(const float3& position, const float3& direction) : mPosition{position}, mDirection{direction}
    { BELL_ASSERT(glm::length(mDirection) == 1.0f, "Direction vector of ray is not normalised") }

    float3 getPosition() const
        { return mPosition; }

    float3 getDirection() const
        { return mDirection; }

private:

    float3 mPosition;
    float3 mDirection;
};


// Free Functions

// Return a vector constructed from the minimum of each component.
inline float3 componentWiseMin(const float3& lhs, const float3& rhs)
{
    return float3{std::min(lhs.x, rhs.x),
                  std::min(lhs.y, rhs.y),
                  std::min(lhs.z, rhs.z)};
}


inline float4 componentWiseMin(const float4& lhs, const float4& rhs)
{
    return float4{std::min(lhs.x, rhs.x),
                  std::min(lhs.y, rhs.y),
                  std::min(lhs.z, rhs.z),
                  std::min(lhs.w, rhs.w)};
}


// Return a vector constructed from the maximum of each component.
inline float3 componentWiseMax(const float3& lhs, const float3& rhs)
{
    return float3{std::max(lhs.x, rhs.x),
                  std::max(lhs.y, rhs.y),
                  std::max(lhs.z, rhs.z)};
}


inline float4 componentWiseMax(const float4& lhs, const float4& rhs)
{
    return float4{std::max(lhs.x, rhs.x),
                  std::max(lhs.y, rhs.y),
                  std::max(lhs.z, rhs.z),
                  std::max(lhs.w, rhs.w)};
}


struct Rect
{
	uint32_t x, y;
};


#endif

