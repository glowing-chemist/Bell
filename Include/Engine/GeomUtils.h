#ifndef GEOMUTILS_HPP
#define GEOMUTILS_HPP

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
	
    Plane(const glm::vec4& position, const glm::vec4& normal) : mCenterPosition{position},
        mNormal{normal} {}

    bool isInFrontOf(const AABB&, const Estimation) const;
    bool isInFrontOf(const float3&) const;
    bool isInFrontOf(const float4&) const;

    float distanceTo(const float3&) const;
    float distanceTo(const float4&) const;

    float4 closestPoint(const float3&) const;
    float4 closestPoint(const float4&) const;

	private:

    float4 mCenterPosition;
	float4 mNormal;
};


class Ray
{
public:
    Ray(const float4& position, const float4& direction) : mPosition{position}, mDirection{direction} {}

private:

    float4 mPosition;
    float4 mDirection;
};

#endif

