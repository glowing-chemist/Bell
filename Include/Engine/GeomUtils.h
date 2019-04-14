#ifndef GEOMUTILS_HPP
#define GEOMUTILS_HPP

#include "Core/BellLogging.hpp"

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using uint2 = glm::vec<2, uint32_t>;
using uint3 = glm::vec<3, uint32_t>;
using uint4 = glm::vec<4, uint32_t>;

using int2 = glm::vec<2, int32_t>;
using int3 = glm::vec<3, int32_t>;
using int4 = glm::vec<4, int32_t>;


class AABB;


enum class EstimationMode
{
    Over,
    Under
};


class Plane
{
	public:
	
    Plane(const glm::vec3& position, const glm::vec3& normal) : mCenterPosition{position},
        mNormal{normal}
        { BELL_ASSERT(glm::length(mNormal) == 1.0f, "Direction vecotr of plane is not normalised") }

    Plane() = default;

    bool isInFrontOf(const AABB&, const EstimationMode) const;
    bool isInFrontOf(const float3&) const;

    float distanceTo(const float3&) const;

    float3 closestPoint(const float3&) const;

	private:

    float3 mCenterPosition;
    float3 mNormal;
};


class Ray
{
public:
    Ray(const float3& position, const float3& direction) : mPosition{position}, mDirection{direction}
    { BELL_ASSERT(glm::length(mDirection) == 1.0f, "Direction vector of ray is not normalised") }

private:

    float3 mPosition;
    float3 mDirection;
};

#endif

