#include "Engine/AABB.hpp"

#include <limits>

std::array<float4, 8> AABB::getCubeAsVertexArray() const
{
    float4 upper1{mMinimum};
    float4 upper2{mMinimum.x, mMinimum.y, mMaximum.z, 1.0f};
    float4 upper3{mMaximum.x, mMinimum.y, mMaximum.z, 1.0f };
    float4 upper4{mMaximum.x, mMinimum.y, mMinimum.z, 1.0f };

    float4 lower1{mMinimum.x, mMaximum.y, mMinimum.z, 1.0f };
    float4 lower2{mMinimum.x, mMaximum.y, mMaximum.z, 1.0f };
    float4 lower3{mMaximum};
    float4 lower4{mMaximum.x, mMaximum.y, mMinimum.z, 1.0f };

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

    // mTopFrontLeft is the corner of AABB with minimal coordinates - left top.
    float t1 = (mMinimum.x - rayOrigin.x) * inverseDirection.x;
    float t2 = (mMaximum.x - rayOrigin.x) * inverseDirection.x;
    float t3 = (mMinimum.y - rayOrigin.y) * inverseDirection.y;
    float t4 = (mMaximum.y - rayOrigin.y) * inverseDirection.y;
    float t5 = (mMinimum.z - rayOrigin.z) * inverseDirection.z;
    float t6 = (mMaximum.z - rayOrigin.z) * inverseDirection.z;

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


bool AABB::contains(const float4& point) const
{
	return point.x >= mMinimum.x && point.x <= mMaximum.x &&
		point.y >= mMinimum.y && point.y <= mMaximum.y &&
		point.z >= mMinimum.z && point.z <= mMaximum.z;
}


bool AABB::contains(const AABB& aabb, const EstimationMode estimationMode) const
{
	const auto verticies = aabb.getCubeAsVertexArray();
	
	bool inside = true;
	for (const auto& vertex : verticies)
	{
		const bool isInside = contains(vertex);
		inside = inside && isInside;

		if (estimationMode == EstimationMode::Over && isInside)
			return isInside;
	}

	return inside;
}


AABB& AABB::operator*=(const float4x4& mat)
{
    // Keep track of the max/min values seen on each axis
    // so tha we still have an AABB not an OOBB.
    const auto cubeVerticies= getCubeAsVertexArray();
    float4 smallest = float4(10000000.0f);
    float4 largest = float4(-10000000.0f);
	for (const auto& vertex : cubeVerticies)
	{
        float4 transformedPoint = mat * vertex;
		smallest = componentWiseMin(smallest, transformedPoint);

		largest = componentWiseMax(largest, transformedPoint);
	}

	mMinimum = smallest;
	mMaximum = largest;

    return *this;
}


AABB& AABB::operator*=(const float4& vec)
{
    mMinimum *= vec;
    mMaximum *= vec;

    return *this;
}


AABB& AABB::operator+=(const float4& vec)
{
    mMinimum += vec;
    mMaximum += vec;

    return *this;
}


AABB& AABB::operator-=(const float4& vec)
{
    mMinimum -= vec;
    mMaximum -= vec;

    return *this;
}


AABB AABB::operator*(const float4x4& mat) const
{
	// Keep track of the max/min values seen on each axis
	// so tha we still have an AABB not an OOBB.
	const auto cubeVerticies = getCubeAsVertexArray();
    float4 smallest = float4(10000000.0f);
    float4 largest = float4(-10000000.0f);
	for (const auto& vertex : cubeVerticies)
	{
        const float4 transformedPoint = mat * vertex;
		smallest = componentWiseMin(smallest, transformedPoint);

		largest = componentWiseMax(largest, transformedPoint);
	}

	return AABB{ smallest, largest };
}


AABB AABB::operator*(const float4& vec) const
{
	return AABB{ mMinimum * vec, mMaximum * vec };
}


AABB AABB::operator+(const float4& vec) const
{
	return AABB{ mMinimum + vec, mMaximum + vec };
}


AABB AABB::operator-(const float4& vec) const
{
	return AABB{ mMinimum - vec, mMaximum - vec };
}


OBB OBB::operator*(const float4& vec) const
{
    Cube newCube = mCube;
    newCube.mLower1 *= vec;
    newCube.mUpper2 *= vec;
    newCube.mUpper3 *= vec;
    newCube.mUpper4 *= vec;
    newCube.mLower1 *= vec;
    newCube.mLower2 *= vec;
    newCube.mLower3 *= vec;
    newCube.mLower4 *= vec;

    return newCube;
}


OBB OBB::operator+(const float4& vec) const
{
    Cube newCube = mCube;
    newCube.mLower1 += vec;
    newCube.mUpper2 += vec;
    newCube.mUpper3 += vec;
    newCube.mUpper4 += vec;
    newCube.mLower1 += vec;
    newCube.mLower2 += vec;
    newCube.mLower3 += vec;
    newCube.mLower4 += vec;

    return newCube;
}


OBB OBB::operator-(const float4& vec) const
{
    Cube newCube = mCube;
    newCube.mLower1 -= vec;
    newCube.mUpper2 -= vec;
    newCube.mUpper3 -= vec;
    newCube.mUpper4 -= vec;
    newCube.mLower1 -= vec;
    newCube.mLower2 -= vec;
    newCube.mLower3 -= vec;
    newCube.mLower4 -= vec;

    return newCube;
}


OBB OBB::operator*(const float4x4& mat) const
{
    Cube newCube = mCube;
    newCube.mLower1 = mat * newCube.mLower1;
    newCube.mUpper2 = mat * newCube.mUpper2;
    newCube.mUpper3 = mat * newCube.mUpper3;
    newCube.mUpper4 = mat * newCube.mUpper4;
    newCube.mLower1 = mat * newCube.mLower1;
    newCube.mLower2 = mat * newCube.mLower2;
    newCube.mLower3 = mat * newCube.mLower3;
    newCube.mLower4 = mat * newCube.mLower4;

    return newCube;
}
