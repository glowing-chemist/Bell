#include "Engine/AABB.hpp"

#include <cmath>
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


// Helper functions.
namespace
{
    bool getSeparatingPlane(const float3& RPos, const float3& plane, const OBB& box1, const OBB& box2)
    {
        const Basis basis1 = box1.getBasisVectors();
        const float3 size1 = box1.getSideLengths();
        const float3 halfVector1 = float3{size1.x / 2.0f, size1.y / 2.0f, size1.z / 2.0f};

        const Basis basis2 = box2.getBasisVectors();
        const float3 size2 = box2.getSideLengths();
        const float3 halfVector2 = float3{size2.x / 2.0f, size2.y / 2.0f, size2.z / 2.0f};

        return (std::fabs(glm::dot(RPos, plane)) >
                (std::fabs(glm::dot(basis1.mX * halfVector1.x, plane)) +
                 std::fabs(glm::dot(basis1.mY * halfVector1.y, plane)) +
                 std::fabs(glm::dot(basis1.mZ * halfVector1.z, plane)) +
                 std::fabs(glm::dot(basis2.mX * halfVector2.x, plane))  +
                 std::fabs(glm::dot(basis2.mY * halfVector2.y, plane)) +
                 std::fabs(glm::dot(basis2.mZ * halfVector2.z, plane))));
    }

    bool getCollision(const OBB& box1, const OBB& box2)
    {
        float3 RPos;
        RPos = box2.getCentralPoint() - box1.getCentralPoint();

        const Basis basis1 = box1.getBasisVectors();
        const Basis basis2 = box2.getBasisVectors();

        return !(getSeparatingPlane(RPos, basis1.mX, box1, box2) ||
                 getSeparatingPlane(RPos, basis1.mY, box1, box2) ||
                 getSeparatingPlane(RPos, basis1.mZ, box1, box2) ||
                 getSeparatingPlane(RPos, basis2.mX, box1, box2) ||
                 getSeparatingPlane(RPos, basis2.mY, box1, box2) ||
                 getSeparatingPlane(RPos, basis2.mZ, box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mX, basis2.mX), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mX, basis2.mY), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mX, basis2.mZ), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mY, basis2.mX), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mY, basis2.mY), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mY, basis2.mZ), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mZ, basis2.mX), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mZ, basis2.mY), box1, box2) ||
                 getSeparatingPlane(RPos, glm::cross(basis1.mZ, basis2.mZ), box1, box2));
    }
}


OBB OBB::operator*(const float4& vec) const
{
    const float3 newStart = mStart * float3(vec);
    const float3 newHalf = mHalfSize * float3(vec);
    return OBB(mBasis, newHalf, newStart);
}


OBB OBB::operator+(const float4& vec) const
{
    float3 newStart = mStart + float3(vec);
    return OBB(mBasis, mHalfSize, newStart);
}


OBB OBB::operator-(const float4& vec) const
{
    float3 newStart = mStart - float3(vec);
    return OBB(mBasis, mHalfSize, newStart);
}


OBB OBB::operator*(const float4x4& mat) const
{
    const float3 newStart = mat * float4(mStart, 1.0f);
    const float3 newHalf = mat * float4(mHalfSize, 0.0f);
    const Basis newBasis = Basis{glm::normalize(mat * float4(mBasis.mX, 0.0f)),
                                 glm::normalize(mat * float4(mBasis.mY, 0.0f)),
                                 glm::normalize(mat * float4(mBasis.mZ, 0.0f))};

    return OBB(newBasis, newHalf, newStart);
}


bool OBB::intersects(const OBB& other) const
{
    return getCollision(*this, other);
}
